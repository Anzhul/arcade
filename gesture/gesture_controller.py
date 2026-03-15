"""
Gesture Controller for Arcade Ship Game (Two-Hand Mode)
Uses MediaPipe Hands to translate hand gestures into game inputs,
then sends them over UDP in the same format as the Feather controller.

Controls:
  Right hand:
    - Wrist X/Y position       -> joystick_x / joystick_y (move ship)
    - Index finger up           -> button1  (fire)
    - All fingers extended      -> button3  (special weapon)
  Left hand:
    - Pinch (thumb+index close) -> button2  (dash / shield)
    - Finger spread distance    -> pot1     (shield/firerate)
    - Wrist tilt (hand angle)   -> pot2     (weapon mode)
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


def hand_bbox(lm, img_w: int, img_h: int, padding: int = 20):
    """Return (x1, y1, x2, y2) bounding box in pixel coords with padding."""
    xs = [l.x * img_w for l in lm]
    ys = [l.y * img_h for l in lm]
    x1 = max(0, int(min(xs)) - padding)
    y1 = max(0, int(min(ys)) - padding)
    x2 = min(img_w, int(max(xs)) + padding)
    y2 = min(img_h, int(max(ys)) + padding)
    return x1, y1, x2, y2


def build_packet(jx, jy, p1, p2, b1, b2, b3) -> bytes:
    data = {
        "joyX": jx, "joyY": jy,
        "pot1": p1, "pot2": p2,
        "btn1": int(b1), "btn2": int(b2), "btn3": int(b3),
    }
    return json.dumps(data, separators=(",", ":")).encode()


# ── Main ───────────────────────────────────────────────────────────────────────

def main(show_preview: bool = True):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Pi 5 uses libcamera — use picamera2 to capture frames
    picam = Picamera2()
    picam.configure(picam.create_preview_configuration(
        main={"format": "RGB888", "size": (640, 480)}
    ))
    picam.start()
    print("Camera started via picamera2.")

    print(f"Sending gestures -> udp://{GAME_IP}:{GAME_PORT}")
    print("Press Q to quit.")

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
            # picamera2 outputs RGB888; MediaPipe expects RGB, OpenCV expects BGR
            rgb = frame
            frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
            result = hands.process(rgb)

            jx, jy = 0, 0
            p1, p2 = 50, 50
            b1 = b2 = b3 = False

            if result.multi_hand_landmarks:
                # Assign hands by wrist x-position in mirrored frame:
                # rightmost wrist = user's right hand
                right_lm = None
                left_lm = None
                detected = [(hl.landmark, hl) for hl in result.multi_hand_landmarks]

                if len(detected) == 1:
                    lm, _ = detected[0]
                    if lm[0].x > 0.5:
                        right_lm = lm
                    else:
                        left_lm = lm
                elif len(detected) >= 2:
                    # Sort by wrist x — rightmost first
                    detected.sort(key=lambda d: d[0][0].x, reverse=True)
                    right_lm = detected[0][0]
                    left_lm = detected[1][0]

                # ── Right hand: movement + attack ─────────────────────────
                if right_lm:
                    wrist = right_lm[0]
                    jx = map_range(wrist.x, 0.15, 0.85, -100, 100)
                    jy = map_range(wrist.y, 0.1,  0.9,  -100, 100)

                    b1 = finger_up(right_lm, 8, 6)              # fire
                    b3 = count_fingers(right_lm) >= 5            # special

                # ── Left hand: defense + analog controls ──────────────────
                if left_lm:
                    pinch_dist = landmark_dist(left_lm[4], left_lm[8])
                    b2 = pinch_dist < 0.05                       # dash/shield

                    spread = landmark_dist(left_lm[4], left_lm[20])
                    p1 = map_range(spread, 0.10, 0.55, 0, 100)

                    dx = left_lm[8].x - left_lm[5].x
                    dy = left_lm[8].y - left_lm[5].y
                    angle = math.degrees(math.atan2(-dy, dx))
                    p2 = map_range(angle, -90, 90, 0, 100)

                # ── Draw both hands in preview ────────────────────────────
                if show_preview:
                    for hand_landmarks in result.multi_hand_landmarks:
                        mp_drawing.draw_landmarks(
                            frame, hand_landmarks,
                            mp_hands.HAND_CONNECTIONS,
                        )

                    for lm, is_right, tag in [
                        (right_lm, True,
                         f"RIGHT  fire={'ON' if b1 else 'off'}  spc={'ON' if b3 else 'off'}"),
                        (left_lm, False,
                         f"LEFT  dash={'ON' if b2 else 'off'}  p1={p1} p2={p2}"),
                    ]:
                        if lm is None:
                            continue
                        color = (0, 255, 0) if is_right else (255, 165, 0)
                        x1, y1, x2, y2 = hand_bbox(lm, w, h)
                        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
                        cv2.putText(frame, tag, (x1, y1 - 6),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.45, color, 2)

            # Send packet every frame
            packet = build_packet(jx, jy, p1, p2, b1, b2, b3)
            sock.sendto(packet, (GAME_IP, GAME_PORT))

            if show_preview:
                num_hands = len(result.multi_hand_landmarks) if result.multi_hand_landmarks else 0
                label = (f"hands={num_hands} | joyX={jx:+4d} joyY={jy:+4d} | "
                         f"p1={p1:3d} p2={p2:3d} | "
                         f"b1={int(b1)} b2={int(b2)} b3={int(b3)}")
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
