cmake_minimum_required(VERSION 3.8)
project(firefighter_robot)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# ROS2 dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)

# OpenCV
find_package(OpenCV REQUIRED)

# PCL
find_package(PCL REQUIRED COMPONENTS common io filters segmentation)
find_package(pcl_conversions REQUIRED)

# ONNX Runtime — set ONNXRUNTIME_ROOT if installed to a custom path
# e.g. cmake .. -DONNXRUNTIME_ROOT=/opt/onnxruntime
if(DEFINED ONNXRUNTIME_ROOT)
  set(ONNXRUNTIME_INCLUDE_DIRS ${ONNXRUNTIME_ROOT}/include)
  set(ONNXRUNTIME_LIBRARIES ${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so)
else()
  find_path(ONNXRUNTIME_INCLUDE_DIRS onnxruntime_cxx_api.h
    PATHS /usr/local/include/onnxruntime /usr/include/onnxruntime)
  find_library(ONNXRUNTIME_LIBRARIES onnxruntime
    PATHS /usr/local/lib /usr/lib)
endif()

if(NOT ONNXRUNTIME_INCLUDE_DIRS OR NOT ONNXRUNTIME_LIBRARIES)
  message(FATAL_ERROR "ONNX Runtime not found. Install it or set -DONNXRUNTIME_ROOT=<path>")
endif()

message(STATUS "ONNX Runtime include: ${ONNXRUNTIME_INCLUDE_DIRS}")
message(STATUS "ONNX Runtime lib:     ${ONNXRUNTIME_LIBRARIES}")

# libcurl
find_package(CURL REQUIRED)

# Common deps list for ament_target_dependencies
set(ROS_DEPS
  rclcpp
  sensor_msgs
  std_msgs
  geometry_msgs
  nav_msgs
  visualization_msgs
  pcl_conversions
)

# ── camera_node ──────────────────────────────────────────────────────────────
add_executable(camera_node src/camera_node.cpp)
ament_target_dependencies(camera_node rclcpp sensor_msgs std_msgs)
target_include_directories(camera_node PRIVATE include)

# ── inference_node ───────────────────────────────────────────────────────────
add_executable(inference_node src/inference_node.cpp)
ament_target_dependencies(inference_node rclcpp sensor_msgs std_msgs visualization_msgs)
target_include_directories(inference_node PRIVATE
  include
  ${ONNXRUNTIME_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(inference_node
  ${ONNXRUNTIME_LIBRARIES}
  ${OpenCV_LIBS}
)

# ── lidar_node ───────────────────────────────────────────────────────────────
add_executable(lidar_node src/lidar_node.cpp)
ament_target_dependencies(lidar_node rclcpp sensor_msgs geometry_msgs visualization_msgs)
target_include_directories(lidar_node PRIVATE include)

# ── decision_node ────────────────────────────────────────────────────────────
add_executable(decision_node src/decision_node.cpp)
ament_target_dependencies(decision_node rclcpp std_msgs geometry_msgs visualization_msgs)
target_include_directories(decision_node PRIVATE include)

# ── telemetry_node ───────────────────────────────────────────────────────────
add_executable(telemetry_node src/telemetry_node.cpp)
ament_target_dependencies(telemetry_node rclcpp std_msgs geometry_msgs)
target_include_directories(telemetry_node PRIVATE include)
target_link_libraries(telemetry_node CURL::libcurl)

# ── update_agent_node ────────────────────────────────────────────────────────
add_executable(update_agent_node src/update_agent_node.cpp)
ament_target_dependencies(update_agent_node rclcpp std_msgs)
target_include_directories(update_agent_node PRIVATE include)
target_link_libraries(update_agent_node CURL::libcurl)

# Install all executables
install(TARGETS
  camera_node
  inference_node
  lidar_node
  decision_node
  telemetry_node
  update_agent_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})

ament_package()
