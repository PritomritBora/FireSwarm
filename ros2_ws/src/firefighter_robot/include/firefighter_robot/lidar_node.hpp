#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <geometry_msgs/msg/pose_array.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <std_msgs/msg/header.hpp>

#include <vector>
#include <cmath>

namespace firefighter_robot
{

struct HumanCandidate {
  float x, y, z;
  float confidence;
};

// Human cluster size thresholds (metres) for 2D LiDAR
constexpr float HUMAN_MIN_WIDTH = 0.3f;
constexpr float HUMAN_MAX_WIDTH = 0.8f;

class LidarNode : public rclcpp::Node
{
public:
  explicit LidarNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  void scanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg);
  std::vector<HumanCandidate> detectHumans(const sensor_msgs::msg::LaserScan & scan);
  void publishMarkers(const std::vector<HumanCandidate> & candidates,
                      const std_msgs::msg::Header & header);

  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr lidar_sub_;
  rclcpp::Publisher<geometry_msgs::msg::PoseArray>::SharedPtr  survivor_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr marker_pub_;

  rclcpp::CallbackGroup::SharedPtr reentrant_group_;

  double cluster_tolerance_;
  int    min_cluster_points_;
  int    max_cluster_points_;
};

}  // namespace firefighter_robot
