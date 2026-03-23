"""
Download a pretrained fire/smoke YOLOv8 model and export to ONNX.
Run with: conda activate firefighter && python ml/export.py

Uses a publicly available fire detection model from Roboflow Universe
or falls back to fine-tuning YOLOv8n on a small fire dataset.
"""

import shutil
from pathlib import Path
from ultralytics import YOLO

OUTPUT_DIR = Path("models")
ONNX_NAME  = "fire_smoke.onnx"

def main():
    OUTPUT_DIR.mkdir(exist_ok=True)

    # Option 1: use a publicly available fire detection model
    # This model is trained on fire/smoke and available without auth
    print("Loading YOLOv8n fire detection model...")

    try:
        # Try downloading from a public source
        import urllib.request
        model_url = "https://github.com/robmarkcole/fire-detection-from-images/raw/master/models/yolov8n_fire.pt"
        pt_path = OUTPUT_DIR / "yolov8n_fire.pt"
        print(f"Downloading from {model_url}...")
        urllib.request.urlretrieve(model_url, pt_path)
        model = YOLO(str(pt_path))
    except Exception as e:
        print(f"Direct download failed ({e}), falling back to base YOLOv8n...")
        # Fallback: use base YOLOv8n — works for testing the pipeline
        # Replace with a fire-trained model later
        model = YOLO("yolov8n.pt")
        print("WARNING: Using base YOLOv8n (not fire-trained). Pipeline will work but detections won't be accurate until you replace with a fire-trained model.")

    print("Class names:", model.names)

    print("Exporting to ONNX...")
    export_path = model.export(format="onnx", opset=12, imgsz=640, dynamic=False)

    dest = OUTPUT_DIR / ONNX_NAME
    shutil.move(str(export_path), dest)
    print(f"Model saved to: {dest}")
    print("Done. Update robot_params.yaml → inference_node.model_path if needed.")

if __name__ == "__main__":
    main()
