from fastapi import FastAPI, Depends, HTTPException
from sqlalchemy.orm import Session
from pydantic import BaseModel
from typing import Optional
import json

from database import SessionLocal, Alert, ModelVersion, init_db

app = FastAPI(title="Firefighter Robot Fleet — Backend")


@app.on_event("startup")
def startup():
    init_db()


def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()


# ── Alert ingestion ────────────────────────────────────────────────────────

class AlertPayload(BaseModel):
    robot_id:  str
    timestamp: float
    type:      str
    level:     str
    data:      dict


@app.post("/alerts", status_code=201)
def receive_alert(payload: AlertPayload, db: Session = Depends(get_db)):
    alert = Alert(
        robot_id=payload.robot_id,
        type=payload.type,
        level=payload.level,
        data=json.dumps(payload.data),
        timestamp=payload.timestamp,
    )
    db.add(alert)
    db.commit()
    return {"status": "ok"}


@app.get("/alerts")
def list_alerts(limit: int = 100, db: Session = Depends(get_db)):
    rows = db.query(Alert).order_by(Alert.id.desc()).limit(limit).all()
    return [
        {"id": r.id, "robot_id": r.robot_id, "type": r.type,
         "level": r.level, "data": json.loads(r.data), "timestamp": r.timestamp}
        for r in rows
    ]


# ── Model registry ─────────────────────────────────────────────────────────

@app.get("/model/latest")
def get_latest_model(db: Session = Depends(get_db)):
    model = db.query(ModelVersion).order_by(ModelVersion.id.desc()).first()
    if not model:
        raise HTTPException(status_code=404, detail="No model registered")
    return {"version": model.version, "url": model.url}


class ModelPayload(BaseModel):
    version: str
    url:     str


@app.post("/model", status_code=201)
def register_model(payload: ModelPayload, db: Session = Depends(get_db)):
    model = ModelVersion(version=payload.version, url=payload.url)
    db.add(model)
    db.commit()
    return {"status": "registered", "version": payload.version}
