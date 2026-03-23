#!/bin/bash
# Start the mapping session — Gazebo + SLAM + RViz in one command
# Teleop runs in a separate terminal (it needs keyboard focus)

set -e

source /opt/ros/jazzy/setup.bash
source ~/Projects/FireFIght/ros2_ws/install/setup.bash

cd ~/Projects/FireFIght

echo "============================================"
echo " Firefighter Robot — Mapping Session"
echo "============================================"
echo ""
echo " In a SEPARATE terminal, run teleop:"
echo "   source /opt/ros/jazzy/setup.bash"
echo "   ros2 run teleop_twist_keyboard teleop_twist_keyboard"
echo ""
echo " Keys: i=forward  ,=back  j=left  l=right  k=stop"
echo " Use w/x to adjust speed (keep it around 0.2)"
echo ""
echo " When done mapping, save with:"
echo "   ros2 run nav2_map_server map_saver_cli -f ~/Projects/FireFIght/simulation/maps/fire_building_map"
echo ""
echo "============================================"
echo " Starting in 3 seconds..."
sleep 3

ros2 launch firefighter_robot mapping.launch.py
