# Implementation Plan — Firefighter Robot Fleet

## Environment Setup (Conda)

ROS2 and Conda don't play perfectly together out of the box, so we use a specific approach:
- ROS2 Humble installed system-wide (via apt on Ubuntu 22.04)
- Conda environment for all Python ML/backend work (inference, telemetry, dashboard)
- ROS2 nodes written in Python use the conda env's interpreter via a wrapper

### System Requirements
- Ubuntu 22.04 (required for ROS2 Humble)
- Conda (Miniconda recommended)
- GPU optional but helpful for inference

---

## Step 0 — Base Installation

### 1. Install ROS2 Humble (system-wide, not in conda)
```bash
sudo apt update && sudo apt install -y software-properties-common
sudo add-apt-repository universe
sudo apt update && sudo apt install -y curl
curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
  -o /usr/share/keyrings/ros-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) \
  signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] \
  http://packages.ros.org/ros2/ubuntu \
  $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | \
  sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null
sudo apt update
sudo apt install -y ros-humble-desktop ros-humble-gazebo-ros-pkgs \
  ros-humble-nav2-bringup ros-humble-turtlebot3-gazebo \
  python3-colcon-common-extensions
```

### 2. Create Conda Environment
```bash
conda create -n firefighter python=3.10 -y
conda activate firefighter
```

### 3. Install Python dependencies in conda env
```bash
pip install ultralytics opencv-python numpy open3d \
  fastapi uvicorn sqlalchemy plotly dash \
  mlflow requests pydantic
```

### 4. Install ROS2 Python bindings into conda env
```bash
pip install catkin_pkg empy lark
# Source ROS2 in every terminal session
echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
```

### 5. Verify setup
```bash
source /opt/ros/humble/setup.bash
conda activate firefighter
ros2 --version
python -c "import ultralytics; print('YOLOv8 ready')"
```

---

## Language Split

| Component | Language | Reason |
|---|---|---|
| All ROS2 robot nodes | C++ (rclcpp) | Low latency, production standard |
| Inference (YOLOv8) | C++ + ONNX Runtime | Hot path — every camera frame goes through here |
| LiDAR processing | C++ + PCL | Point Cloud Library is C++ native |
| Telemetry HTTP calls | C++ + libcurl | Stays in the same node, no IPC overhead |
| FastAPI backend | Python | No latency requirement |
| Plotly Dash dashboard | Python | No latency requirement |
| MLflow / model export | Python | Tooling is Python-native |

### Inference Pipeline
- Train/fine-tune YOLOv8 in Python (ultralytics)
- Export to ONNX once: `yolo export model=yolov8n.pt format=onnx opset=12`
- C++ inference node loads `.onnx` at startup via ONNX Runtime C++ API
- OpenCV handles frame preprocessing (resize, normalize, BGR→RGB)
- Optional: enable TensorRT execution provider in ONNX Runtime for GPU acceleration

---

## Project Structure

```
firefighter-robot-fleet/
├── ros2_ws/                          # ROS2 workspace
│   └── src/
│       └── firefighter_robot/        # Single C++ ROS2 package
│           ├── src/
│           │   ├── camera_node.cpp
│           │   ├── inference_node.cpp   # ONNX Runtime + OpenCV
│           │   ├── lidar_node.cpp       # PCL clustering
│           │   ├── decision_node.cpp
│           │   ├── telemetry_node.cpp   # libcurl HTTP POST
│           │   └── update_agent_node.cpp
│           ├── include/firefighter_robot/
│           │   ├── inference_node.hpp
│           │   └── lidar_node.hpp
│           ├── launch/
│           │   ├── single_robot.launch.py
│           │   └── fleet.launch.py
│           ├── config/
│           │   └── robot_params.yaml
│           ├── CMakeLists.txt
│           └── package.xml
├── backend/                          # FastAPI telemetry server (Python)
│   ├── main.py
│   ├── models.py
│   └── database.py
├── dashboard/                        # Plotly Dash frontend (Python)
│   └── app.py
├── ml/                               # Model training + export (Python)
│   ├── train.py
│   ├── export.py
│   └── models/                       # .onnx model weights
├── simulation/                       # Gazebo world files
│   ├── worlds/
│   │   └── fire_building.world
│   └── models/
│       └── fire_object/
├── environment.yml                   # Conda env spec (Python tools)
└── README.md
```

---

## Phase 1 — Single Robot Core Perception
**Duration: 2–3 weeks**
**Goal: one robot moving, detecting fire/smoke and humans, sending alerts to dashboard**

### Milestone 1.1 — Simulation Environment
- [ ] Create Gazebo world: indoor building with rooms and corridors
- [ ] Add fire/smoke mesh objects in 2-3 rooms
- [ ] Add a human mesh in a smoke-obscured room
- [ ] Spawn TurtleBot3 (or equivalent) with camera + LiDAR attached

### Milestone 1.2 — Camera + Fire Detection
- [ ] `camera_node.py` — subscribes to `/camera/image_raw`, republishes frames
- [ ] `inference_node.py` — runs YOLOv8 on frames, publishes `/detections`
- [ ] Fine-tune YOLOv8n on FLAME dataset (or use pretrained as baseline)
- [ ] Output: bounding boxes with class (fire/smoke) and confidence

### Milestone 1.3 — LiDAR Human Detection
- [ ] `lidar_node.py` — subscribes to `/scan` or `/points`, republishes
- [ ] `pointcloud_node.py` — Euclidean clustering on PointCloud2
- [ ] Filter clusters by human-sized bounding volume (0.4–0.7m wide, 1.4–1.9m tall)
- [ ] Publish `/survivor_candidates` with position + confidence
- [ ] Demo: camera feed obscured by smoke, LiDAR still detects human shape

### Milestone 1.4 — Decision Node
- [ ] `decision_node.py` — subscribes to `/detections` and `/survivor_candidates`
- [ ] Assigns hazard level: LOW / MEDIUM / HIGH / SURVIVOR
- [ ] Publishes `/alerts` topic with structured alert messages

### Milestone 1.5 — Telemetry + Backend
- [ ] `telemetry_node.py` — POSTs alerts to FastAPI backend
- [ ] FastAPI backend receives and stores alerts in SQLite
- [ ] Basic Dash dashboard: live alert feed table

**Phase 1 demo:** robot patrols, camera detects fire, LiDAR detects human through smoke, alert appears on dashboard.

---

## Phase 2 — Autonomous Navigation
**Duration: 1–2 weeks**
**Goal: robot explores the building intelligently without human control**

### Milestone 2.1 — Nav2 Integration
- [ ] Configure Nav2 stack for the robot
- [ ] Provide floor plan as a static map
- [ ] Robot builds local costmap as it moves

### Milestone 2.2 — Frontier Exploration
- [ ] Implement frontier-based exploration (moves toward unexplored map edges)
- [ ] Robot systematically covers all rooms without revisiting cleared areas
- [ ] Exploration stops when full map is covered or all survivors found

### Milestone 2.3 — Hazard-Aware Navigation
- [ ] Robot avoids cells marked as fire in its costmap
- [ ] Replans route if fire is detected ahead

**Phase 2 demo:** robot autonomously explores entire building, avoids fire zones, covers all rooms.

---

## Phase 3 — Shared Situational Map
**Duration: 1–2 weeks**
**Goal: live color-coded floor plan on dashboard showing fire, safe zones, survivors**

### Milestone 3.1 — Occupancy Map Fusion
- [ ] Each robot publishes its local occupancy grid to backend
- [ ] Backend merges grids: fire / clear / unknown / survivor-possible per cell
- [ ] Map updates in real-time as robots report

### Milestone 3.2 — Dashboard Map View
- [ ] Dash renders fused occupancy grid as color-coded floor plan
- [ ] Fire cells = red, clear = green, unknown = grey, survivor = yellow
- [ ] Robot positions shown as icons on the map
- [ ] Map auto-refreshes every 2 seconds

**Phase 3 demo:** live floor plan showing fire spreading, safe corridors, survivor location marked.

---

## Phase 4 — Multi-Robot Fleet
**Duration: 1 week**
**Goal: 3–5 robots running in parallel with coordinated coverage**

### Milestone 4.1 — Multi-Robot Launch
- [ ] `fleet.launch.py` spawns N robots with unique namespaces (`/robot1`, `/robot2`, ...)
- [ ] Each robot runs its own full node graph
- [ ] All robots POST to the same backend

### Milestone 4.2 — Coordinated Exploration
- [ ] Robots share explored zones — no redundant coverage
- [ ] Backend assigns frontier zones to robots to maximize coverage speed
- [ ] Dashboard shows per-robot status (active / lost comms / low battery sim)

**Phase 4 demo:** 3 robots deployed simultaneously, building fully mapped in 1/3 the time of a single robot.

---

## Phase 5 — Model Update System
**Duration: 1 week**
**Goal: push new ML model to robots live without stopping them**

### Milestone 5.1 — MLflow Registry
- [ ] Run MLflow local server
- [ ] Register model versions with metadata (accuracy, date, hazard classes)
- [ ] Script to push new model version to registry

### Milestone 5.2 — Update Agent Node
- [ ] `update_agent_node.py` polls MLflow every N minutes
- [ ] On new version detected: downloads weights, signals inference node to reload
- [ ] Inference node hot-reloads model, other nodes unaffected
- [ ] Logs update event to backend

**Phase 5 demo:** push new model version, robots update live, inference continues without interruption.

---

## Phase 6 — Realism & Polish
**Duration: 1 week**
**Goal: make the simulation more convincing and robust**

### Milestone 6.1 — Smoke Occlusion
- [ ] Add Gazebo smoke particle effects that visually block camera
- [ ] Confirm LiDAR still detects human through smoke volume
- [ ] Side-by-side demo: camera feed (black) vs LiDAR map (human visible)

### Milestone 6.2 — Fire Spread Simulation
- [ ] Fire objects expand over time (scripted in Gazebo)
- [ ] Robots track moving fire boundary
- [ ] Dashboard shows fire spreading in real-time

### Milestone 6.3 — Comms Dropout
- [ ] Simulate network loss for a robot (drop telemetry for N seconds)
- [ ] Robot stores detections locally during dropout
- [ ] On reconnect, replays buffered telemetry to backend
- [ ] Dashboard shows robot as "comms lost" then recovers

---

## Running the System

```bash
# Terminal 1 — ROS2 + Gazebo
source /opt/ros/humble/setup.bash
cd ros2_ws
colcon build
source install/setup.bash
ros2 launch firefighter_robot single_robot.launch.py

# Terminal 2 — Backend
conda activate firefighter
cd backend
uvicorn main:app --reload --port 8000

# Terminal 3 — Dashboard
conda activate firefighter
cd dashboard
python app.py

# Terminal 4 — MLflow (Phase 5)
conda activate firefighter
mlflow server --port 5000
```

---

## Conda Environment File (environment.yml)

```yaml
name: firefighter
channels:
  - defaults
  - conda-forge
dependencies:
  - python=3.10
  - pip
  - pip:
    - ultralytics
    - opencv-python
    - numpy
    - open3d
    - fastapi
    - uvicorn
    - sqlalchemy
    - plotly
    - dash
    - mlflow
    - requests
    - pydantic
    - catkin_pkg
    - empy
    - lark
```

Recreate anytime with:
```bash
conda env create -f environment.yml
conda activate firefighter
```
