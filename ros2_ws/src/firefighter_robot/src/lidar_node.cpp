#include "firefighter_robot/lidar_node.hpp"

#include <geometry_msgs/msg/pose.hpp>
#include <visualization_msgs/msg/marker.hpp>

#include <cmath>
#include <vector>

namespace firefighter_robot
{

LidarNode::LidarNode(const rclcpp::NodeOptions & options)
: Node("lidar_node", options)
{
  this->declare_parameter("lidar_topic",         "/scan");
  this->declare_parameter("survivor_topic",      "/robot/survivor_candidates");
  this->declare_parameter("marker_topic",        "/robot/survivor_markers");
  this->declare_parameter("cluster_tolerance",   0.3);
  this->declare_parameter("min_cluster_points",  3);
  this->declare_parameter("max_cluster_points",  50);

  cluster_tolerance_  = this->get_parameter("cluster_tolerance").as_double();
  min_cluster_points_ = this->get_parameter("min_cluster_points").as_int();
  max_cluster_points_ = this->get_parameter("max_cluster_points").as_int();

  reentrant_group_ = this->create_callback_group(
    rclcpp::CallbackGroupType::Reentrant);

  rclcpp::SubscriptionOptions sub_options;
  sub_options.callback_group = reentrant_group_;

  lidar_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
    this->get_parameter("lidar_topic").as_string(),
    rclcpp::SensorDataQoS(),
    std::bind(&LidarNode::scanCallback, this, std::placeholders::_1),
    sub_options);

  survivor_pub_ = this->create_publisher<geometry_msgs::msg::PoseArray>(
    this->get_parameter("survivor_topic").as_string(), 10);

  marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
    this->get_parameter("marker_topic").as_string(), 10);

  RCLCPP_INFO(this->get_logger(), "LidarNode ready — 2D LaserScan human detection active");
}

void LidarNode::scanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg)
{
  auto candidates = detectHumans(*msg);

  geometry_msgs::msg::PoseArray pose_array;
  pose_array.header = msg->header;
  for (const auto & c : candidates) {
    geometry_msgs::msg::Pose p;
    p.position.x = c.x;
    p.position.y = c.y;
    p.position.z = 0.0;
    p.orientation.w = 1.0;
    pose_array.poses.push_back(p);
  }
  survivor_pub_->publish(pose_array);
  publishMarkers(candidates, msg->header);
}

std::vector<HumanCandidate> LidarNode::detectHumans(
  const sensor_msgs::msg::LaserScan & scan)
{
  std::vector<HumanCandidate> candidates;

  // Convert scan to 2D Cartesian points, skip invalid ranges
  struct Point2D { float x, y; };
  std::vector<Point2D> points;
  for (size_t i = 0; i < scan.ranges.size(); ++i) {
    float r = scan.ranges[i];
    if (r < scan.range_min || r > scan.range_max) continue;
    float angle = scan.angle_min + i * scan.angle_increment;
    points.push_back({r * std::cos(angle), r * std::sin(angle)});
  }

  if (points.empty()) return candidates;

  // Simple 2D Euclidean clustering
  std::vector<bool> visited(points.size(), false);
  for (size_t i = 0; i < points.size(); ++i) {
    if (visited[i]) continue;

    // Grow cluster from point i
    std::vector<size_t> cluster;
    cluster.push_back(i);
    visited[i] = true;

    for (size_t j = i + 1; j < points.size(); ++j) {
      if (visited[j]) continue;
      float dx = points[j].x - points[cluster.back()].x;
      float dy = points[j].y - points[cluster.back()].y;
      if (std::sqrt(dx * dx + dy * dy) < cluster_tolerance_) {
        cluster.push_back(j);
        visited[j] = true;
      }
    }

    int n = static_cast<int>(cluster.size());
    if (n < min_cluster_points_ || n > max_cluster_points_) continue;

    // Compute bounding box width of cluster
    float min_x = points[cluster[0]].x, max_x = min_x;
    float min_y = points[cluster[0]].y, max_y = min_y;
    float cx = 0, cy = 0;
    for (size_t idx : cluster) {
      min_x = std::min(min_x, points[idx].x);
      max_x = std::max(max_x, points[idx].x);
      min_y = std::min(min_y, points[idx].y);
      max_y = std::max(max_y, points[idx].y);
      cx += points[idx].x;
      cy += points[idx].y;
    }
    cx /= n; cy /= n;

    float width = std::sqrt(
      (max_x - min_x) * (max_x - min_x) +
      (max_y - min_y) * (max_y - min_y));

    // Human-sized cluster in 2D
    if (width >= HUMAN_MIN_WIDTH && width <= HUMAN_MAX_WIDTH) {
      HumanCandidate c;
      c.x = cx; c.y = cy; c.z = 0.9f;  // assume standing height centroid
      c.confidence = 0.7f;
      candidates.push_back(c);
      RCLCPP_WARN(this->get_logger(),
        "Survivor candidate at (%.2f, %.2f) width=%.2fm", cx, cy, width);
    }
  }

  return candidates;
}

void LidarNode::publishMarkers(
  const std::vector<HumanCandidate> & candidates,
  const std_msgs::msg::Header & header)
{
  visualization_msgs::msg::MarkerArray array;
  int id = 0;
  for (const auto & c : candidates) {
    visualization_msgs::msg::Marker m;
    m.header      = header;
    m.ns          = "survivor";
    m.id          = id++;
    m.type        = visualization_msgs::msg::Marker::CYLINDER;
    m.action      = visualization_msgs::msg::Marker::ADD;
    m.pose.position.x = c.x;
    m.pose.position.y = c.y;
    m.pose.position.z = 0.9f;
    m.pose.orientation.w = 1.0;
    m.scale.x = 0.5; m.scale.y = 0.5; m.scale.z = 1.7;
    m.color.a = 0.8f;
    m.color.r = 1.0f; m.color.g = 1.0f; m.color.b = 0.0f;
    array.markers.push_back(m);
  }
  marker_pub_->publish(array);
}

}  // namespace firefighter_robot

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<firefighter_robot::LidarNode>();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
  return 0;
}
