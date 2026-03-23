"""
Download a pretrained fire/smoke YOLOv8 model and export to ONNX.
Run with: conda activate firefighter && python ml/export.py
"""

import shutil
from pathlib import Path
from ultralytics import YOLO

OUTPUT_DIR = Path("models")
ONNX_NAME  = "fire_smoke.onnx"

def main():
    OUTPUT_DIR.mkdir(exist_ok=True)

    # Download pretrained fire detection model from HuggingFace
    print("Downloading pretrained fire/smoke model...")
    from huggingface_hub import hf_hub_download
    weights_path = hf_hub_download(
        repo_id="keremberke/yolov8n-fire-detection",
        filename="best.pt"
    )
    print(f"Downloaded weights to: {weights_path}")

    model = YOLO(weights_path)
    print("Class names:", model.names)

    # Export to ONNX
    print("Exporting to ONNX...")
    export_path = model.export(format="onnx", opset=12, imgsz=640, dynamic=False)

    dest = OUTPUT_DIR / ONNX_NAME
    shutil.move(export_path, dest)
    print(f"Model saved to: {dest}")
    print("Done. Use this path in robot_params.yaml → inference_node.model_path")

if __name__ == "__main__":
    main()
