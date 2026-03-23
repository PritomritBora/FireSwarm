# Project Status — Firefighter Robot Fleet
Last updated: 2026-03-23

## Overall Progress
Phase 1 (core pipeline) in progress. Simulation environment running, sensors verified, C++ nodes built.

---

## What's Done

### Infrastructure
- [x] Docker + Docker Compose set up
- [x] Backend container built and running (port 8001)
- [x] Dashboard container built and running (port 8050)
- [x] Backend accepts and stores alerts (`POST /alerts`)
- [x] Backend serves model registry (`GET /model/latest`, `POST /model`)
- [x] Dashboard polls backend every 2s and renders live alert table
- [x] ROS2 Jazzy installed on host
- [x] Gazebo Sim 8.10.0 installed
- [x] PCL, OpenCV, libcurl, ONNX Runtime installed
- [x] C++ package builds clean (`colcon build`) — twice, including after lidar refactor

### Simulation
- [x] Gazebo world SDF (`simulation/worlds/fire_building.sdf`)
  - 4 rooms + central corridor
  - 2 fire objects with point lights
  - Smoke volume in bottom-right room
  - 2 human-shaped survivor models
  - 2 debris obstacles
  - TurtleBot3 Waffle spawned at building entrance
- [x] ROS-Gazebo bridge running (`ros_gz_bridge`)
- [x] `/scan` (LaserScan) verified publishing data
- [x] `/camera/image_raw` bridge active
- [x] `/cmd_vel`, `/odom`, `/tf` bridged

### C++ ROS2 Package (`ros2_ws/src/firefighter_robot`)
- [x] `camera_node.cpp` — passthrough, ready for preprocessing
- [x] `inference_node.cpp` — YOLOv8 ONNX Runtime, multithreaded executor + reentrant callback group
- [x] `lidar_node.cpp` — refactored to 2D LaserScan clustering (no PCL), human shape detection by cluster width
- [x] `decision_node.cpp` — fuses detections, assigns LOW/MEDIUM/HIGH/SURVIVOR levels
- [x] `telemetry_node.cpp` — libcurl HTTP POST to backend
- [x] `update_agent_node.cpp` — polls model registry, hot reload signal
- [x] `single_robot.launch.py`
- [x] `robot_params.yaml`

### Python Backend (`backend/`)
- [x] FastAPI + SQLite
- [x] `POST /alerts`, `GET /alerts`
- [x] `GET /model/latest`, `POST /model`

### Dashboard (`dashboard/`)
- [x] Plotly Dash, live alert table, color-coded hazard levels
- [x] Summary bar, auto-refresh every 2s

---

## In Progress

- [ ] ONNX model export — conda env + pretrained YOLOv8 fire detection
- [ ] Wire inference node to `/camera/image_raw` topic
- [ ] Wire lidar node to `/scan` topic and test human detection end-to-end
- [ ] Add ros_gz_bridge to launch file (currently run manually)

---

## Not Started

### Phase 2 — Autonomous Navigation
- [ ] Nav2 stack configuration
- [ ] Frontier-based exploration
- [ ] Hazard-aware route replanning

### Phase 3 — Shared Situational Map
- [ ] Occupancy grid fusion at backend
- [ ] Floor plan map view on dashboard

### Phase 4 — Multi-Robot Fleet
- [ ] Multi-robot launch file
- [ ] Coordinated zone assignment

### Phase 5 — Model Update System
- [ ] MLflow container wired up
- [ ] Update agent end-to-end test

### Phase 6 — Realism
- [ ] Smoke particle effects (camera occlusion)
- [ ] Fire spread simulation
- [ ] Comms dropout + store-and-forward

### ML
- [ ] Conda env setup
- [ ] Pretrained YOLOv8 fire model download + ONNX export
- [ ] Test model on sample fire images

---

## Services

| Service   | URL                   | Status      |
|-----------|-----------------------|-------------|
| Backend   | http://localhost:8001 | running     |
| Dashboard | http://localhost:8050 | running     |
| MLflow    | http://localhost:5000 | not started |

---

## Next Steps (priority order)
1. Set up conda env, download pretrained fire model, export to ONNX
2. Add ros_gz_bridge to launch file
3. Run all nodes together and verify alerts flow to dashboard
4. Test lidar human detection with robot near survivor models
