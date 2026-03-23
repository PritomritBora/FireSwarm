from sqlalchemy import create_engine, Column, Integer, String, Float, DateTime
from sqlalchemy.orm import declarative_base, sessionmaker
from datetime import datetime

DATABASE_URL = "sqlite:///data/alerts.db"

engine = create_engine(DATABASE_URL, connect_args={"check_same_thread": False})
SessionLocal = sessionmaker(bind=engine)
Base = declarative_base()


class Alert(Base):
    __tablename__ = "alerts"
    id         = Column(Integer, primary_key=True, index=True)
    robot_id   = Column(String)
    type       = Column(String)
    level      = Column(String)
    data       = Column(String)   # JSON blob
    timestamp  = Column(Float)
    created_at = Column(DateTime, default=datetime.utcnow)


class ModelVersion(Base):
    __tablename__ = "model_versions"
    id         = Column(Integer, primary_key=True, index=True)
    version    = Column(String, unique=True)
    url        = Column(String)
    created_at = Column(DateTime, default=datetime.utcnow)


def init_db():
    import os
    os.makedirs("data", exist_ok=True)
    Base.metadata.create_all(bind=engine)
