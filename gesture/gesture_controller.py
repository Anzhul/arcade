"""
Gesture Controller for Arcade Ship Game
Uses MediaPipe Hands to translate hand gestures into game inputs,
then sends them over UDP in the same format as the Feather controller.

Controls:
  - Hand X position  -> joystick_x  (move ship left/right)
  - Hand Y position  -> joystick_y  (move ship up/down)
  - Index finger up  -> button1     (fire)
  - Pinch (thumb+index close) -> button2  (dash / shield)
  - All fingers extended       -> button3  (special weapon)
  - Finger spread distance     -> pot1     (shield/firerate)
  - Wrist tilt (hand angle)    -> pot2     (weapon mode)
"""

import cv2
import mediapipe as mp
import socket
import json
import math
import argparse
import numpy as np
from picamera2 import Picamera2

# ── UDP settings ───────────────────────────────────────────────────────────────
GAME_IP   = "127.0.0.1"
GAME_PORT = 8888

# ── MediaPipe setup ────────────────────────────────────────────────────────────
mp_hands   = mp.solutions.hands
mp_drawing = mp.solutions.drawing_utils

# ── Helpers ────────────────────────────────────────────────────────────────────

def landmark_dist(a, b) -> float:
    return math.hypot(a.x - b.x, a.y - b.y)


def finger_up(lm, tip_id: int, pip_id: int) -> bool:
    """Returns True if the finger is extended (tip above PIP joint in image space)."""
    return lm[tip_id].y < lm[pip_id].y


def count_fingers(lm) -> int:
    fingers = 0
    # Thumb: compare tip x to IP joint x (flipped for right hand)
    if lm[4].x < lm[3].x:
        fingers += 1
    # Four fingers
    for tip, pip in [(8, 6), (12, 10), (16, 14), (20, 18)]:
        if finger_up(lm, tip, pip):
            fingers += 1
    return fingers


def map_range(value: float, in_lo: float, in_hi: float,
              out_lo: int, out_hi: int) -> int:
    """Linear map, clamped."""
    ratio = (value - in_lo) / (in_hi - in_lo)
    ratio = max(0.0, min(1.0, ratio))
    return int(out_lo + ratio * (out_hi - out_lo))


def build_packet(jx, jy, p1, p2, b1, b2, b3, b4=False) -> bytes:
    data = {
        "joyX": jx, "joyY": jy,
        "pot1": p1, "pot2": p2,
        "btn1": int(b1), "btn2": int(b2), "btn3": int(b3), "btn4": int(b4),
    }
    return json.dumps(data, separators=(",", ":")).encode()


# ── Main ───────────────────────────────────────────────────────────────────────

def main(show_preview: bool = True):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Pi 5 uses libcamera — use picamera2 to capture frames
    picam = Picamera2()
    picam.configure(picam.create_preview_configuration(
        main={"format": "XRGB8888", "size": (640, 480)}
    ))
    picam.start()
    print("Camera started via picamera2.")

    print(f"Sending gestures -> udp://{GAME_IP}:{GAME_PORT}")
    print("Press Q to quit.")

    prev_snap    = False  # edge detection for weapon cycle gesture
    fist_anchor  = None   # wrist X when fist was first made
    fist_p1      = 50     # last pot1 value set by fist drag
    pinch_origin = None   # (x, y) midpoint when left pinch was established

    with mp_hands.Hands(
        max_num_hands=2,
        min_detection_confidence=0.7,
        min_tracking_confidence=0.6,
    ) as hands:
        while True:
            frame = picam.capture_array()

            # Flip so it acts as a mirror
            frame = cv2.flip(frame, 1)
            h, w = frame.shape[:2]
            # XRGB8888 on Pi 5 (little-endian) = BGRX in memory → take first 3 bytes = BGR
            frame = np.ascontiguousarray(frame[:, :, :3])    # drop X → (h, w, 3) BGR
            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)     # RGB for MediaPipe
            result = hands.process(rgb)

            jx, jy = 0, 0
            p1, p2 = 50, 50
            b1 = b2 = b3 = b4 = False

            if result.multi_hand_landmarks:
                # Build a dict: "Left" / "Right" → landmark list
                hand_map = {}
                for hand_lm, handedness in zip(
                    result.multi_hand_landmarks, result.multi_handedness
                ):
                    label = handedness.classification[0].label  # "Left" or "Right"
                    hand_map[label] = hand_lm

                # ── Left hand: movement ───────────────────────────────────
                if "Left" in hand_map:
                    lm = hand_map["Left"].landmark
                    PINCH_THRESH = 0.06

                    # index+thumb pinch acts as virtual joystick
                    left_pinch = landmark_dist(lm[4], lm[8]) < PINCH_THRESH
                    if left_pinch:
                        mx = (lm[4].x + lm[8].x) / 2  # midpoint between tips
                        my = (lm[4].y + lm[8].y) / 2
                        if pinch_origin is None:
                            pinch_origin = (mx, my)     # lock origin on first pinch frame
                        # ±0.2 normalised units of travel maps to ±100
                        jx = max(-100, min(100, int((mx - pinch_origin[0]) / 0.2 * 100)))
                        jy = max(-100, min(100, int((my - pinch_origin[1]) / 0.2 * 100)))
                    else:
                        pinch_origin = None             # release clears origin
                        jx, jy = 0, 0

                    # pot2: wrist tilt via index MCP→tip angle
                    dx = lm[8].x - lm[5].x
                    dy = lm[8].y - lm[5].y
                    angle = math.degrees(math.atan2(-dy, dx))
                    p2 = map_range(angle, -90, 90, 0, 100)
                else:
                    pinch_origin = None

                # ── Right hand: actions ────────────────────────────────────
                if "Right" in hand_map:
                    lm = hand_map["Right"].landmark
                    PINCH_THRESH = 0.06

                    # pot1: fist drag — hold fist and slide left/right
                    is_fist = count_fingers(lm) == 0
                    if is_fist:
                        if fist_anchor is None:
                            fist_anchor = lm[0].x   # lock anchor on first fist frame
                        delta = lm[0].x - fist_anchor
                        # ±0.3 hand-width of travel maps to ±50 around last resting value
                        fist_p1 = max(0, min(100, fist_p1 + int(delta * 300)))
                        fist_anchor = lm[0].x       # slide anchor with movement
                    else:
                        fist_anchor = None          # release anchor when fist opens
                    p1 = fist_p1

                    # buttons only fire when not in a fist
                    if not is_fist:
                        b1 = landmark_dist(lm[4], lm[8])  < PINCH_THRESH  # index+thumb  → fire
                        b2 = landmark_dist(lm[4], lm[12]) < PINCH_THRESH  # middle+thumb → dash/shield
                        b3 = landmark_dist(lm[4], lm[16]) < PINCH_THRESH  # ring+thumb   → special

                        # pinky+thumb → cycle weapon (edge-triggered)
                        pinky_pinch = landmark_dist(lm[4], lm[20]) < PINCH_THRESH
                        b4 = pinky_pinch and not prev_snap
                        prev_snap = pinky_pinch
                    else:
                        prev_snap = False
                else:
                    prev_snap = False

                if show_preview:
                    for hand_lm in result.multi_hand_landmarks:
                        mp_drawing.draw_landmarks(
                            frame, hand_lm, mp_hands.HAND_CONNECTIONS,
                        )
                    if pinch_origin is not None and "Left" in hand_map:
                        lm = hand_map["Left"].landmark
                        ox = int(pinch_origin[0] * w)
                        oy = int(pinch_origin[1] * h)
                        cx = int((lm[4].x + lm[8].x) / 2 * w)
                        cy = int((lm[4].y + lm[8].y) / 2 * h)
                        cv2.circle(frame, (ox, oy), 10, (0, 255, 255), -1)   # origin: yellow
                        cv2.circle(frame, (cx, cy),  8, (0, 128, 255), -1)   # current: orange
                        cv2.line(frame, (ox, oy), (cx, cy), (255, 255, 255), 1)

            # Send packet every frame
            packet = build_packet(jx, jy, p1, p2, b1, b2, b3, b4)
            sock.sendto(packet, (GAME_IP, GAME_PORT))

            if show_preview:
                label = (f"joyX={jx:+4d} joyY={jy:+4d} | "
                         f"p1={p1:3d} p2={p2:3d} | "
                         f"b1={int(b1)} b2={int(b2)} b3={int(b3)} b4={int(b4)}")
                cv2.putText(frame, label, (8, 24),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 0), 2)
                cv2.imshow("Gesture Controller", frame)

            if cv2.waitKey(1) & 0xFF == ord("q"):
                break

    picam.stop()
    cv2.destroyAllWindows()
    sock.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MediaPipe gesture controller for arcade game")
    parser.add_argument("--ip",      default=GAME_IP,   help="Game host IP (default: 127.0.0.1)")
    parser.add_argument("--port",    default=GAME_PORT,  type=int, help="UDP port (default: 8888)")
    parser.add_argument("--no-preview", action="store_true", help="Disable camera preview window")
    args = parser.parse_args()

    GAME_IP   = args.ip
    GAME_PORT = args.port
    main(show_preview=not args.no_preview)
