"""
Hailo-Accelerated Gesture Controller for Arcade Ship Game
Runs YOLOv8 pose estimation on the Hailo-8L NPU to extract body keypoints,
then maps arm/wrist positions to game inputs sent over UDP.

Controls:
  - Right wrist X/Y position      -> joystick_x / joystick_y (move ship)
  - Right wrist above shoulder     -> button1 (fire)
  - Both wrists raised above hips  -> button2 (shield/dash)
  - Right wrist above nose         -> button3 (special weapon)
  - Right forearm angle            -> pot1 (shield/firerate)
  - Wrist horizontal spread        -> pot2 (weapon mode)

COCO keypoint indices used:
  0=nose  5=L_shoulder  6=R_shoulder
  7=L_elbow  8=R_elbow  9=L_wrist  10=R_wrist
  11=L_hip   12=R_hip
"""

import cv2
import numpy as np
import socket
import json
import math
import argparse
from picamera2 import Picamera2
from hailo_platform import HEF, VDevice, FormatType, HailoSchedulingAlgorithm
from hailo_platform import InputVStreamParams, OutputVStreamParams, InferVStreams

# ── Config ─────────────────────────────────────────────────────────────────────
GAME_IP    = "127.0.0.1"
GAME_PORT  = 8888
HEF_PATH   = "/usr/share/hailo-models/yolov8s_pose_h8l_pi.hef"
MODEL_SIZE = 640          # Model input resolution
CONF_THRESH = 0.5         # Person detection confidence threshold
KPT_CONF    = 0.3         # Keypoint visibility threshold

# YOLOv8 pose output: 3 heads at strides 8, 16, 32
STRIDES    = [8, 16, 32]
GRID_SIZES = [80, 40, 20]
REG_MAX    = 16           # DFL bins

# COCO keypoint indices
NOSE        = 0
L_SHOULDER  = 5;  R_SHOULDER = 6
L_ELBOW     = 7;  R_ELBOW    = 8
L_WRIST     = 9;  R_WRIST    = 10
L_HIP       = 11; R_HIP      = 12

# ── Helpers ─────────────────────────────────────────────────────────────────────

def map_range(value, in_lo, in_hi, out_lo, out_hi):
    r = (value - in_lo) / max(in_hi - in_lo, 1e-6)
    return int(max(out_lo, min(out_hi, out_lo + r * (out_hi - out_lo))))


def build_packet(jx, jy, p1, p2, b1, b2, b3):
    data = {"joyX": jx, "joyY": jy,
            "pot1": p1, "pot2": p2,
            "btn1": int(b1), "btn2": int(b2), "btn3": int(b3)}
    return json.dumps(data, separators=(",", ":")).encode()


# ── YOLOv8 Pose post-processing ─────────────────────────────────────────────────

def dfl_decode(pred_dist):
    """Decode DFL box regression. pred_dist: (..., 4*REG_MAX) -> (..., 4)."""
    b = pred_dist.shape[:-1]
    pred_dist = pred_dist.reshape(*b, 4, REG_MAX)
    # Softmax then weighted sum over REG_MAX bins
    e = np.exp(pred_dist - pred_dist.max(axis=-1, keepdims=True))
    e /= e.sum(axis=-1, keepdims=True)
    proj = np.arange(REG_MAX, dtype=np.float32)
    return (e * proj).sum(axis=-1)  # (..., 4) = (l, t, r, b)


def make_anchors(stride, grid_h, grid_w):
    """Return anchor center points for a given stride."""
    yi, xi = np.meshgrid(np.arange(grid_h), np.arange(grid_w), indexing="ij")
    return np.stack([xi + 0.5, yi + 0.5], axis=-1).reshape(-1, 2).astype(np.float32)


def decode_head(box_raw, obj_raw, kpt_raw, stride):
    """
    box_raw : (H, W, 4*REG_MAX)
    obj_raw : (H, W, 1)
    kpt_raw : (H, W, 17*3)
    Returns arrays of shape (N, 4) boxes (x1y1x2y2 in pixel space),
                             (N,)   scores,
                             (N, 17, 3) keypoints (x, y, conf) in pixel space.
    """
    H, W = box_raw.shape[:2]
    anchors = make_anchors(stride, H, W)          # (H*W, 2)

    scores = 1 / (1 + np.exp(-obj_raw.reshape(-1)))   # sigmoid

    # DFL box decode
    dist = dfl_decode(box_raw.reshape(-1, 4 * REG_MAX))  # (H*W, 4)
    # dist = (l, t, r, b) in grid units; convert to pixel coords
    lt = anchors - dist[:, :2]
    rb = anchors + dist[:, 2:]
    boxes = np.concatenate([lt, rb], axis=1) * stride  # pixel coords

    # Keypoints
    kpts = kpt_raw.reshape(-1, 17, 3).copy()
    kpts[..., 0] = (kpts[..., 0] * 2 + anchors[:, 0:1]) * stride
    kpts[..., 1] = (kpts[..., 1] * 2 + anchors[:, 1:2]) * stride
    kpts[..., 2] = 1 / (1 + np.exp(-kpts[..., 2]))     # sigmoid visibility

    return boxes, scores, kpts


def nms(boxes, scores, iou_thresh=0.45):
    """Simple NMS, returns kept indices."""
    if len(boxes) == 0:
        return []
    x1, y1, x2, y2 = boxes[:, 0], boxes[:, 1], boxes[:, 2], boxes[:, 3]
    areas = (x2 - x1) * (y2 - y1)
    order = scores.argsort()[::-1]
    keep = []
    while order.size > 0:
        i = order[0]
        keep.append(i)
        ix1 = np.maximum(x1[i], x1[order[1:]])
        iy1 = np.maximum(y1[i], y1[order[1:]])
        ix2 = np.minimum(x2[i], x2[order[1:]])
        iy2 = np.minimum(y2[i], y2[order[1:]])
        inter = np.maximum(0, ix2 - ix1) * np.maximum(0, iy2 - iy1)
        iou = inter / (areas[i] + areas[order[1:]] - inter + 1e-6)
        order = order[1:][iou < iou_thresh]
    return keep


def decode_outputs(outputs, conf_thresh=CONF_THRESH):
    """
    outputs: dict mapping layer name -> numpy array
    Returns list of (box_x1y1x2y2, keypoints[17,3]) for each detected person.
    """
    head_data = []
    for stride, gs in zip(STRIDES, GRID_SIZES):
        # Find the three tensors for this head by shape
        box_t = obj_t = kpt_t = None
        for name, arr in outputs.items():
            if arr.shape == (gs, gs, 4 * REG_MAX):
                box_t = arr
            elif arr.shape == (gs, gs, 1):
                obj_t = arr
            elif arr.shape == (gs, gs, 51):
                kpt_t = arr
        if box_t is None:
            continue
        head_data.append(decode_head(box_t, obj_t, kpt_t, stride))

    all_boxes  = np.concatenate([h[0] for h in head_data], axis=0)
    all_scores = np.concatenate([h[1] for h in head_data], axis=0)
    all_kpts   = np.concatenate([h[2] for h in head_data], axis=0)

    mask = all_scores >= conf_thresh
    all_boxes, all_scores, all_kpts = all_boxes[mask], all_scores[mask], all_kpts[mask]

    keep = nms(all_boxes, all_scores)
    return list(zip(all_boxes[keep], all_kpts[keep]))


# ── Gesture mapping ─────────────────────────────────────────────────────────────

def kp(keypoints, idx):
    """Return (x, y, conf) for keypoint idx."""
    return keypoints[idx]


def visible(keypoints, *indices):
    return all(kp(keypoints, i)[2] >= KPT_CONF for i in indices)


def gestures_to_input(keypoints, frame_w, frame_h):
    """Map body keypoints to InputState values."""
    jx, jy = 0, 0
    p1, p2 = 50, 50
    b1 = b2 = b3 = False

    # ── Joystick: right wrist position ────────────────────────────────────────
    if visible(keypoints, R_WRIST):
        wx, wy, _ = kp(keypoints, R_WRIST)
        # Mirror (camera is flipped) — already handled by frame flip
        jx = map_range(wx, frame_w * 0.1, frame_w * 0.9, -100, 100)
        jy = map_range(wy, frame_h * 0.1, frame_h * 0.9, -100, 100)

    # ── Button 1: right wrist above right shoulder (raise arm to fire) ────────
    if visible(keypoints, R_WRIST, R_SHOULDER):
        b1 = kp(keypoints, R_WRIST)[1] < kp(keypoints, R_SHOULDER)[1]

    # ── Button 2: both wrists raised above hips ───────────────────────────────
    if visible(keypoints, L_WRIST, R_WRIST, L_HIP, R_HIP):
        hip_y = (kp(keypoints, L_HIP)[1] + kp(keypoints, R_HIP)[1]) / 2
        b2 = (kp(keypoints, L_WRIST)[1] < hip_y and
              kp(keypoints, R_WRIST)[1] < hip_y)

    # ── Button 3: right wrist above nose (punch-up gesture) ───────────────────
    if visible(keypoints, R_WRIST, NOSE):
        b3 = kp(keypoints, R_WRIST)[1] < kp(keypoints, NOSE)[1]

    # ── Pot 1: right forearm angle (elbow→wrist) ──────────────────────────────
    if visible(keypoints, R_ELBOW, R_WRIST):
        ex, ey, _ = kp(keypoints, R_ELBOW)
        wx, wy, _ = kp(keypoints, R_WRIST)
        angle = math.degrees(math.atan2(ey - wy, wx - ex))  # 0°=right, 90°=up
        p1 = map_range(angle, -90, 90, 0, 100)

    # ── Pot 2: horizontal spread between wrists ───────────────────────────────
    if visible(keypoints, L_WRIST, R_WRIST):
        spread = abs(kp(keypoints, R_WRIST)[0] - kp(keypoints, L_WRIST)[0])
        p2 = map_range(spread, 50, frame_w * 0.8, 0, 100)

    return jx, jy, p1, p2, b1, b2, b3


# ── Drawing ─────────────────────────────────────────────────────────────────────

SKELETON = [
    (R_SHOULDER, R_ELBOW), (R_ELBOW, R_WRIST),
    (L_SHOULDER, L_ELBOW), (L_ELBOW, L_WRIST),
    (R_SHOULDER, L_SHOULDER), (R_HIP, L_HIP),
    (R_SHOULDER, R_HIP), (L_SHOULDER, L_HIP),
]

def draw_pose(frame, keypoints, color=(0, 255, 0)):
    for a, b in SKELETON:
        if keypoints[a][2] >= KPT_CONF and keypoints[b][2] >= KPT_CONF:
            pa = (int(keypoints[a][0]), int(keypoints[a][1]))
            pb = (int(keypoints[b][0]), int(keypoints[b][1]))
            cv2.line(frame, pa, pb, color, 2)
    for i, (x, y, c) in enumerate(keypoints):
        if c >= KPT_CONF:
            cv2.circle(frame, (int(x), int(y)), 4, (0, 200, 255), -1)


# ── Main ────────────────────────────────────────────────────────────────────────

def main(show_preview=True):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # ── Camera ────────────────────────────────────────────────────────────────
    picam = Picamera2()
    picam.configure(picam.create_preview_configuration(
        main={"format": "RGB888", "size": (MODEL_SIZE, MODEL_SIZE)}
    ))
    picam.start()
    print("Camera started via picamera2.")

    # ── Hailo setup ───────────────────────────────────────────────────────────
    hef = HEF(HEF_PATH)
    devices = VDevice()
    network_groups = devices.configure(hef)
    ng = network_groups[0]

    in_params  = InputVStreamParams.make(ng,  quantized=False, format_type=FormatType.FLOAT32)
    out_params = OutputVStreamParams.make(ng, quantized=False, format_type=FormatType.FLOAT32)

    input_info  = hef.get_input_vstream_infos()[0]
    output_info = hef.get_output_vstream_infos()

    print(f"Hailo model ready: {input_info.name} {input_info.shape}")
    print(f"Sending gestures -> udp://{GAME_IP}:{GAME_PORT}")
    print("Controls: move wrist=joystick | raise arm=fire | both arms up=shield | punch up=special")
    print("Press Q to quit.")

    with InferVStreams(ng, in_params, out_params) as pipeline:
        while True:
            # ── Capture ───────────────────────────────────────────────────────
            frame = picam.capture_array()           # RGB, (640, 640, 3)
            frame = cv2.flip(frame, 1)              # mirror
            h, w  = frame.shape[:2]

            # ── Preprocess ────────────────────────────────────────────────────
            inp = frame.astype(np.float32) / 255.0  # normalize to [0,1]
            inp = inp[np.newaxis, ...]               # (1, 640, 640, 3)

            # ── Hailo inference ───────────────────────────────────────────────
            input_data = {input_info.name: inp}
            with pipeline.infer(input_data) as job:
                raw_outputs = job.get()  # dict: name -> (1, H, W, C)

            # Remove batch dim
            outputs = {name: arr[0] for name, arr in raw_outputs.items()}

            # ── Post-process ──────────────────────────────────────────────────
            detections = decode_outputs(outputs)

            jx, jy, p1, p2, b1, b2, b3 = 0, 0, 50, 50, False, False, False

            if detections:
                # Use the highest-confidence detection (first after NMS)
                _, keypoints = detections[0]
                jx, jy, p1, p2, b1, b2, b3 = gestures_to_input(keypoints, w, h)

            # ── Send UDP ──────────────────────────────────────────────────────
            sock.sendto(build_packet(jx, jy, p1, p2, b1, b2, b3), (GAME_IP, GAME_PORT))

            # ── Preview ───────────────────────────────────────────────────────
            if show_preview:
                disp = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
                if detections:
                    _, kpts = detections[0]
                    draw_pose(disp, kpts)
                label = (f"joyX={jx:+4d} joyY={jy:+4d} | "
                         f"p1={p1:3d} p2={p2:3d} | "
                         f"b1={int(b1)} b2={int(b2)} b3={int(b3)}")
                cv2.putText(disp, label, (8, 24),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 0), 2)
                cv2.imshow("Hailo Gesture Controller", disp)

            if cv2.waitKey(1) & 0xFF == ord("q"):
                break

    picam.stop()
    cv2.destroyAllWindows()
    sock.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Hailo-accelerated gesture controller")
    parser.add_argument("--ip",         default=GAME_IP,  help="Game host IP")
    parser.add_argument("--port",       default=GAME_PORT, type=int)
    parser.add_argument("--no-preview", action="store_true")
    args = parser.parse_args()

    GAME_IP   = args.ip
    GAME_PORT = args.port
    main(show_preview=not args.no_preview)
