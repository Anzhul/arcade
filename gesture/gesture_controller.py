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
        max_num_hands=1,
        min_detection_confidence=0.7,
        min_tracking_confidence=0.6,
    ) as hands:
        while True:
            frame = picam.capture_array()

            # Flip so it acts as a mirror
            frame = cv2.flip(frame, 1)
            h, w = frame.shape[:2]
            # picamera2 outputs RGB888; MediaPipe expects RGB, OpenCV display expects BGR
            rgb = frame
            frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
            result = hands.process(rgb)

            jx, jy = 0, 0
            p1, p2 = 50, 50
            b1 = b2 = b3 = False

            if result.multi_hand_landmarks:
                lm = result.multi_hand_landmarks[0].landmark

                # ── Joystick: wrist position mapped to ±100 ────────────────
                wrist = lm[0]
                jx = map_range(wrist.x, 0.15, 0.85, -100, 100)
                jy = map_range(wrist.y, 0.1,  0.9,  -100, 100)

                # ── Buttons ────────────────────────────────────────────────
                index_up = finger_up(lm, 8, 6)
                pinch_dist = landmark_dist(lm[4], lm[8])  # thumb-to-index
                b1 = index_up                              # fire
                b2 = pinch_dist < 0.05                     # dash/shield
                b3 = count_fingers(lm) >= 5               # all open = special

                # ── Potentiometers ─────────────────────────────────────────
                # pot1: spread of all finger tips (proxy for hand openness)
                spread = landmark_dist(lm[4], lm[20])     # thumb to pinky tip
                p1 = map_range(spread, 0.10, 0.55, 0, 100)

                # pot2: angle of index MCP→tip vector (wrist tilt proxy)
                dx = lm[8].x - lm[5].x
                dy = lm[8].y - lm[5].y
                angle = math.degrees(math.atan2(-dy, dx))  # 0°=right, 90°=up
                p2 = map_range(angle, -90, 90, 0, 100)

                if show_preview:
                    mp_drawing.draw_landmarks(
                        frame, result.multi_hand_landmarks[0],
                        mp_hands.HAND_CONNECTIONS,
                    )

            # Send packet every frame
            packet = build_packet(jx, jy, p1, p2, b1, b2, b3)
            sock.sendto(packet, (GAME_IP, GAME_PORT))

            if show_preview:
                label = (f"joyX={jx:+4d} joyY={jy:+4d} | "
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
