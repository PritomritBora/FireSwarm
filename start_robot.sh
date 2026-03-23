#!/bin/bash
# Start the full robot pipeline — Gazebo + all ROS2 nodes + backend + dashboard

set -e

source /opt/ros/jazzy/setup.bash
source ~/Projects/FireFIght/ros2_ws/install/setup.bash

cd ~/Projects/FireFIght

# Check map exists
if [ ! -f "simulation/maps/fire_building_map.yaml" ]; then
  echo "ERROR: Map not found at simulation/maps/fire_building_map.yaml"
  echo "Run ./start_mapping.sh first to create the map."
  exit 1
fi

echo "============================================"
echo " Firefighter Robot Fleet — Full Pipeline"
echo "============================================"
echo ""
echo " Backend:   http://localhost:8001"
echo " Dashboard: http://localhost:8050"
echo ""
echo "============================================"

# Start backend + dashboard in Docker
echo "[1/2] Starting backend and dashboard..."
docker compose up backend dashboard -d

echo "[2/2] Launching robot simulation..."
ros2 launch firefighter_robot single_robot.launch.py
