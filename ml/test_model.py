"""
Quick sanity check — run the exported ONNX model on a test image.
Run with: conda activate firefighter && python ml/test_model.py --image <path>
"""

import argparse
import cv2
import numpy as np
from ultralytics import YOLO

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--image",  default=None, help="Path to test image")
    parser.add_argument("--model",  default="models/fire_smoke.onnx")
    parser.add_argument("--conf",   type=float, default=0.45)
    args = parser.parse_args()

    model = YOLO(args.model, task="detect")

    if args.image:
        results = model(args.image, conf=args.conf)
    else:
        # Use a sample fire image from the web if no image provided
        import urllib.request
        url = "https://upload.wikimedia.org/wikipedia/commons/thumb/8/8e/Campfire_at_dawn.jpg/640px-Campfire_at_dawn.jpg"
        urllib.request.urlretrieve(url, "/tmp/test_fire.jpg")
        results = model("/tmp/test_fire.jpg", conf=args.conf)

    for r in results:
        print(f"Detections: {len(r.boxes)}")
        for box in r.boxes:
            cls  = int(box.cls)
            conf = float(box.conf)
            name = model.names[cls]
            print(f"  {name}: {conf:.2f}  bbox: {box.xyxy[0].tolist()}")
        r.save(filename="ml/test_output.jpg")
        print("Saved annotated image to ml/test_output.jpg")

if __name__ == "__main__":
    main()
