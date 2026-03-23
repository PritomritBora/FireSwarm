# Firefighter Robot Fleet

A simulated multi-robot system for fire mapping and through-smoke survivor detection. Robots patrol a burning building, detect fire/smoke via YOLOv8 on camera, detect survivors via LiDAR point cloud clustering (which works through smoke where cameras fail), and stream live alerts to a command dashboard.

```
[Gazebo Simulation]
       ↓
  Camera Node → Inference Node (YOLOv8 ONNX) ──┐
  LiDAR Node  → Human Shape Clustering        ──┤
                                                 ↓
                                          Decision Node
                                                 ↓
                                         Telemetry Node → FastAPI Backend → Plotly Dash
                                                                ↑
                                         Update Agent Node ─────┘
                                         (polls for new model weights, hot-reloads)
```

All ROS2 nodes are C++ (rclcpp). Backend and dashboard are Python. ML export is Python + ultralytics.

---

## Why LiDAR for survivor detection?

Cameras are blind in smoke. LiDAR is not. A human body has a consistent 3D geometry (~0.5m wide, 1.6–1.8m tall) that appears in a point cloud regardless of smoke, darkness, or heat haze. The LiDAR node clusters 2D laser scan returns and filters by human-sized bounding dimensions to flag survivor candidates — even when the camera feed is completely obscured.

---

## Stack

| Layer | Technology |
|---|---|
| Robotics framework | ROS2 Jazzy |
| Simulator | Gazebo Sim 8 |
| Fire/smoke detection | YOLOv8 via ONNX Runtime (C++) |
| Human detection | 2D LaserScan Euclidean clustering (C++) |
| Backend | FastAPI + SQLite |
| Dashboard | Plotly Dash |
| Model registry | FastAPI `/model` endpoint (MLflow planned) |
| Containerization | Docker + Docker Compose |

---

## Project Structure

```
firefighter-robot-fleet/
├── ros2_ws/src/firefighter_robot/
│   ├── src/
│   │   ├── camera_node.cpp          # image passthrough / preprocessing
│   │   ├── inference_node.cpp       # YOLOv8 ONNX Runtime, multithreaded
│   │   ├── lidar_node.cpp           # 2D scan clustering, human shape detection
│   │   ├── decision_node.cpp        # fuses detections → LOW/MEDIUM/HIGH/SURVIVOR
│   │   ├── telemetry_node.cpp       # libcurl HTTP POST to backend
│   │   └── update_agent_node.cpp    # polls model registry, signals hot reload
│   ├── include/firefighter_robot/
│   ├── launch/single_robot.launch.py
│   ├── config/robot_params.yaml
│   └── CMakeLists.txt
├── backend/                         # FastAPI telemetry + model registry
├── dashboard/                       # Plotly Dash live alert view
├── ml/                              # YOLOv8 export to ONNX
├── simulation/worlds/               # Gazebo SDF world (4 rooms, fire, survivors)
├── models/                          # ONNX weights (mount as volume)
├── data/                            # SQLite database (persisted via volume)
└── docker-compose.yml
```

---

## Quick Start

### Option A — Docker Compose (backend + dashboard + MLflow)

```bash
docker compose up --build
```

| Service   | URL                    |
|-----------|------------------------|
| Backend   | http://localhost:8001  |
| Dashboard | http://localhost:8050  |
| MLflow    | http://localhost:5000  |

The robot container requires Gazebo and a display, so it's best run on the host (see Option B).

### Option B — Host (full system with simulation)

**Prerequisites:** ROS2 Jazzy, Gazebo Sim 8, PCL, OpenCV, libcurl, ONNX Runtime

**1. Export the ONNX model**

```bash
pip install ultralytics
python ml/export.py
# outputs models/fire_smoke.onnx
```

**2. Build the ROS2 workspace**

```bash
cd ros2_ws
colcon build --cmake-args -DONNXRUNTIME_ROOT=/opt/onnxruntime
source install/setup.bash
```

**3. Start backend and dashboard**

```bash
# Terminal 1
cd backend && uvicorn main:app --port 8001

# Terminal 2
cd dashboard && python app.py
```

**4. Launch Gazebo + robot nodes**

```bash
# Terminal 3
source /opt/ros/jazzy/setup.bash
source ros2_ws/install/setup.bash
ros2 launch firefighter_robot single_robot.launch.py
```

**5. Start the ROS-Gazebo bridge (if not in launch file)**

```bash
ros2 run ros_gz_bridge parameter_bridge \
  /scan@sensor_msgs/msg/LaserScan@gz.msgs.LaserScan \
  /camera/image_raw@sensor_msgs/msg/Image@gz.msgs.Image
```

---

## ROS2 Topics

| Topic | Type | Description |
|---|---|---|
| `/robot/camera/image` | `sensor_msgs/Image` | Camera feed |
| `/robot/detections` | `visualization_msgs/MarkerArray` | Fire/smoke bounding boxes |
| `/scan` | `sensor_msgs/LaserScan` | LiDAR scan |
| `/robot/survivor_candidates` | `geometry_msgs/PoseArray` | Human-shaped cluster positions |
| `/robot/survivor_markers` | `visualization_msgs/MarkerArray` | RViz cylinders for survivors |
| `/robot/alerts` | `std_msgs/String` | JSON alert messages |
| `/robot/reload_model` | `std_msgs/String` | Triggers inference node hot reload |

---

## Backend API

```
POST /alerts          — ingest alert from robot
GET  /alerts?limit=N  — fetch recent alerts
GET  /model/latest    — get latest registered model version + download URL
POST /model           — register a new model version
```

---

## Model Updates (live, no downtime)

The `update_agent_node` polls `GET /model/latest` every 60 seconds. When a new version is detected, it downloads the ONNX weights and publishes the path to `/robot/reload_model`. The inference node subscribes to that topic and hot-reloads the ONNX session — other nodes are unaffected.

To push a new model:

```bash
curl -X POST http://localhost:8001/model \
  -H "Content-Type: application/json" \
  -d '{"version": "v2", "url": "http://your-host/fire_smoke_v2.onnx"}'
```

---

## Simulation World

`simulation/worlds/fire_building.sdf` contains:
- 4 rooms + central corridor
- 2 fire objects with point lights
- Smoke volume in the bottom-right room
- 2 human-shaped survivor models
- 2 debris obstacles
- TurtleBot3 Waffle spawned at the building entrance

---

## Development Status

Phase 1 (core pipeline) is in progress. See [project-status.md](project-status.md) for a detailed checklist. Planned phases: autonomous Nav2 navigation, shared occupancy map, multi-robot fleet, MLflow integration, smoke occlusion demo, and comms dropout simulation.
