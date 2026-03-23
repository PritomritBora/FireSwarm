# Project Status — Firefighter Robot Fleet
Last updated: 2026-03-23

## Overall Progress
Phase 1 complete. Phase 2 (autonomous navigation) blocked on Nav2 cmd_vel routing issue.

---

## What's Done

### Infrastructure
- [x] Docker + Docker Compose — backend (port 8001) + dashboard (port 8050) running
- [x] Backend: FastAPI + SQLite, `/alerts` and `/model/latest` endpoints
- [x] Dashboard: Plotly Dash, live alert table, color-coded hazard levels, auto-refresh
- [x] ROS2 Jazzy + Gazebo Sim 8.10.0 installed
- [x] PCL, OpenCV, libcurl, ONNX Runtime installed
- [x] C++ package builds clean

### Simulation
- [x] Gazebo world — fire building with 4 rooms, corridors, fire objects, smoke, 2 survivors, debris
- [x] TurtleBot3 Waffle spawned at building entrance
- [x] ROS-Gazebo bridge — `/scan`, `/camera/image_raw`, `/cmd_vel`, `/odom`, `/tf`, `/joint_states`, `/clock`
- [x] Full TF chain: `map → odom → base_footprint → base_link → base_scan`
- [x] SLAM mapping completed — map saved (`simulation/maps/fire_building_map.pgm` + `.yaml`)

### C++ ROS2 Nodes (all build and run)
- [x] `camera_node` — passthrough
- [x] `inference_node` — YOLOv8 ONNX Runtime, multithreaded
- [x] `lidar_node` — 2D LaserScan clustering, human shape detection
- [x] `decision_node` — hazard levels + survivor deduplication
- [x] `telemetry_node` — libcurl POST to backend
- [x] `update_agent_node` — model registry polling + hot reload
- [x] `patrol_node` — timed waypoint patrol (written, not yet wired in)

### End-to-End Verified (single_robot.launch.py)
- [x] LiDAR detects human-shaped clusters
- [x] Decision node classifies as SURVIVOR with coordinates
- [x] Alerts POST to backend
- [x] Dashboard shows live alerts with deduplication

### ML
- [x] Conda env (`firefighter`) set up
- [x] YOLOv8n exported to ONNX (`models/fire_smoke.onnx`)
- [ ] Fire-trained model (currently base YOLOv8n)

---

## Current Blocker — Nav2 cmd_vel Routing

Nav2 navigation is partially working but the robot doesn't move. Root cause identified:

**The problem:**
Nav2 bringup (Jazzy) routes velocity commands through a pipeline:
```
controller_server → /cmd_vel_nav → velocity_smoother → /cmd_vel_smoothed
```
But the Gazebo bridge listens on `/cmd_vel`. The `/cmd_vel_smoothed` → `/cmd_vel` relay isn't working reliably.

**What works:**
- All Nav2 nodes activate (manually — autostart broken)
- AMCL localizes correctly after initial pose set
- bt_navigator accepts goals
- planner_server computes paths
- controller_server computes velocity commands (verified on `/cmd_vel_smoothed` at 20Hz)
- `/cmd_vel_smoothed` has correct values (linear.x: 0.26, angular.z: 0.47)

**What doesn't work:**
- `/cmd_vel` stays empty — relay node not forwarding
- Robot doesn't move in Gazebo
- Nav2 lifecycle autostart fails — nodes must be activated manually each time
- RTPS shared memory errors on every terminal (leftover from crashed sessions)

**Things to try:**
1. Remap `/cmd_vel_smoothed` to `/cmd_vel` directly in the Nav2 bringup launch args
2. Use `ros2 run topic_tools relay` reliably or replace with a static remap
3. Fix Nav2 autostart — lifecycle manager not activating nodes on startup
4. Clean RTPS shared memory: `sudo rm -rf /dev/shm/fastrtps_*`

---

## Alternative Approach — Patrol Node (no Nav2)

`patrol_node.cpp` is written and ready. It moves the robot through a timed sequence of forward/turn commands — no Nav2, no map needed. This is simpler, more reliable for demo, and already works with the existing bridge setup.

To use it: add `patrol_node` to `single_robot.launch.py` and it will autonomously patrol the building.

---

## Not Started

### Phase 3 — Shared Situational Map
- [ ] Occupancy grid fusion at backend
- [ ] Floor plan map view on dashboard

### Phase 4 — Multi-Robot Fleet
- [ ] Multi-robot launch file
- [ ] Coordinated zone assignment

### Phase 5 — Model Update System
- [ ] MLflow container wired up
- [ ] Fire-trained ONNX model
- [ ] Update agent end-to-end test

### Phase 6 — Realism
- [ ] Smoke particle effects
- [ ] Fire spread simulation
- [ ] Comms dropout + store-and-forward

### Phase 7 — Isaac Sim Migration
- [ ] Install Isaac Sim (NVIDIA GPU available — RTX 3060)
- [ ] Rebuild environment, swap Gazebo launch

---

## Services

| Service   | URL                   | Status      |
|-----------|-----------------------|-------------|
| Backend   | http://localhost:8001 | running     |
| Dashboard | http://localhost:8050 | running     |
| MLflow    | http://localhost:5000 | not started |

---

## Next Steps (priority order)
1. Fix Nav2 cmd_vel routing — try remapping `/cmd_vel_smoothed` → `/cmd_vel` in launch args
2. OR: wire patrol_node into single_robot.launch.py as the navigation solution
3. Fix Nav2 lifecycle autostart
4. Clean RTPS shared memory issues
5. Get fire-trained ONNX model
6. Add patrol_node to full pipeline and verify end-to-end with moving robot
