# Simulated Firefighter Robot Fleet — Project Report

## 1. Project Title
**LiDAR-Based Multi-Robot System for Fire Mapping and Through-Smoke Survivor Detection**

---

## 2. Objective

Design and simulate a multi-robot system deployed in hazardous fire environments that:
- Maps fire boundaries and safe zones in real-time using collaborative perception
- Detects potential survivors through smoke using LiDAR point cloud analysis — where cameras fail
- Communicates actionable intelligence to a central command dashboard
- Supports live ML model updates without downtime

The system is fully simulated but demonstrates a production-grade robotics, perception, and deployment pipeline.

---

## 3. Problem Statement

Firefighters operate in dangerous, low-visibility, and rapidly changing environments. Key challenges include:

- Cameras and human vision fail in smoke, darkness, and heat haze
- No real-time map of where fire is spreading vs. where it's safe
- Survivors may be unconscious or unable to signal their location
- Sending humans in first to assess the situation puts lives at risk
- AI models need to be updated as new hazard patterns are learned

This project addresses the **intelligence layer** of fire search and rescue (FSAR) — assuming hardened robot platforms (like Thermite RS3 or Boston Dynamics Spot) handle the physical environment. Our contribution is the software system that makes those platforms useful.

---

## 4. Core Differentiator

> **Through-smoke human detection using LiDAR**

Cameras are blind in smoke. LiDAR is not.

A human body has a distinctive 3D geometry — roughly 0.5m wide, 0.3m deep, 1.6–1.8m tall — that appears in a point cloud regardless of smoke, darkness, or heat haze. By clustering LiDAR returns and filtering by human-shaped geometry, robots can flag potential survivor locations even when the camera feed is completely obscured.

This is a real technique used in FSAR research and directly addresses the most critical limitation of camera-only systems.

---

## 5. Real-World Hardware Context

This project simulates the software stack. In a real deployment, robots would be hardened platforms capable of surviving fire environments:

| Platform | Notes |
|---|---|
| Thermite RS3 | Used by US fire departments, drives into burning buildings |
| Colossus (Shark Robotics) | Deployed in the 2019 Notre Dame cathedral fire |
| Boston Dynamics Spot | Used for hazard inspection with thermal cameras |

These platforms use heat-resistant chassis, insulated electronics, water-cooled sensors, and sealed enclosures (IP67/IP69). The hardware problem is solved at the industrial level. This project solves the intelligence layer on top.

---

## 6. System Architecture

```
[Central Command Dashboard]
         ↑         ↓
+----------------------------+
|     Fleet of Robots        |
|  Robot 1, Robot 2, ...     |
+----------------------------+
        ↑            ↑
  [Camera Node]  [LiDAR Node]
        ↓            ↓
     [Inference Node]
     - Fire/smoke detection (YOLOv8 on RGB)
     - Human shape detection (point cloud clustering)
        ↓
  [Decision Node]
  - Hazard prioritization
  - Survivor flagging
  - Safe zone calculation
        ↓
  [Telemetry Node] → [FastAPI Backend] → [Dashboard]
        ↑
  [Update Agent Node] ← Cloud Model Registry (MLflow)
```

---

## 7. Robot Objective

The robots are **autonomous scout platforms**, not firefighters. Their mission loop:

1. Patrol the environment on a planned or reactive route
2. Camera node detects fire and smoke continuously (YOLOv8)
3. LiDAR node scans for human-shaped point cloud clusters
4. Decision node fuses detections → classifies zones as fire / safe / survivor-possible
5. Telemetry node streams findings to backend
6. Dashboard updates the live floor map in real-time

Multiple robots cover different zones simultaneously. The dashboard aggregates all observations into a single operational picture:
- Where fire is and where it's spreading
- Where safe corridors exist
- Where potential survivors have been detected

---

## 8. LiDAR Human Detection — Technical Approach

**Why LiDAR works when cameras don't:**
LiDAR measures distance via laser pulses and is unaffected by smoke, darkness, or heat haze. A human body produces a consistent 3D signature in the point cloud.

**Detection pipeline:**
1. Receive `sensor_msgs/PointCloud2` from simulated LiDAR
2. Apply Euclidean clustering to segment objects
3. Filter clusters by bounding box dimensions (human-sized volumes)
4. Optionally classify with a lightweight PointNet-style model
5. Publish survivor candidate locations with confidence scores

**In simulation:**
- Spawn human meshes in Gazebo
- Add smoke particle effects that obscure the camera feed
- Demonstrate that LiDAR still resolves the human shape
- Mark survivor candidates on the dashboard map

---

## 9. Collaborative Fire Boundary Mapping

Each robot contributes observations to a shared occupancy map:
- Cells marked as **fire** (fire/smoke detected)
- Cells marked as **clear** (no hazard detected)
- Cells marked as **unknown** (not yet visited)
- Cells marked as **survivor-possible** (human shape detected)

The backend fuses observations from all robots into a single map. As fire spreads, the map updates in real-time. The dashboard renders this as a live floor plan — the key operational output for a human incident commander.

---

## 10. Implementation Plan

### Phase 1 — Core Pipeline (single robot)
- ROS2 + Gazebo environment with fire objects and a human mesh
- Camera node → YOLOv8 inference → fire/smoke detections
- LiDAR node → point cloud clustering → human shape detection
- Telemetry node → FastAPI backend
- Basic dashboard showing live alerts

### Phase 2 — Model Update System
- MLflow local server as model registry
- Update agent node polls for new weights, reloads inference without stopping other nodes
- Demo: push a new model version, robot updates live

### Phase 3 — Multi-Robot Fleet
- Launch 3–5 robots with different patrol zones
- Shared occupancy map built from all robots' observations
- Dashboard shows per-robot status + aggregated floor map

### Phase 4 — Polish (time permitting)
- Degraded communication simulation (robot loses connection, stores telemetry, replays on reconnect)
- Dynamic evacuation route recommendation based on current fire map
- Thermal camera simulation as additional sensor modality

---

## 11. Tools & Technologies

| Category | Tool |
|---|---|
| Robotics Framework | ROS2 Humble |
| Simulator | Gazebo Classic |
| Fire/Smoke Detection | YOLOv8 (ultralytics) |
| Human Detection | Point cloud clustering + optional PointNet |
| Backend | FastAPI + SQLite |
| Model Registry | MLflow (local) |
| Dashboard | Plotly Dash |
| Datasets | FLAME, Firesense, synthetic Gazebo frames |

---

## 12. Key Challenges Addressed

| Challenge | Approach |
|---|---|
| Camera failure in smoke | LiDAR-based human detection as primary survivor sensor |
| Multi-robot coordination | Shared occupancy map fused at backend |
| Live model updates | Update agent node with safe reload, no downtime |
| Sim-to-real gap | Realistic sensor noise, smoke occlusion, LiDAR artifacts |
| Degraded comms (Phase 4) | Store-and-forward telemetry on reconnect |

---

## 13. Portfolio / Interview Talking Points

- **System thinking**: end-to-end pipeline from raw sensor data to operational dashboard
- **Differentiated perception**: LiDAR human detection through smoke — addresses a real FSAR limitation
- **Deployment pipeline**: live model updates without downtime, not just a research demo
- **Multi-agent coordination**: fleet-level shared situational awareness
- **Honest scoping**: software intelligence layer on top of existing hardened hardware platforms
- **Real-world grounding**: references Thermite RS3, Colossus, actual FSAR research techniques

---

## 14. Demo Script (30-second version)

> "Three robots are patrolling a burning building. The camera feeds are obscured by smoke — you can see that on the left. But the LiDAR is still picking up a human-shaped object in zone C. The dashboard flags it as a potential survivor. Meanwhile the fire boundary map is updating in real-time as the robots report back. An incident commander can see exactly where the fire is, where it's safe, and where someone might be trapped — without sending a person in first."
