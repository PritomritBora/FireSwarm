#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <geometry_msgs/msg/pose_array.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include "firefighter_robot/json.hpp"
#include <chrono>
#include <string>

using json = nlohmann::json;

// Hazard levels
enum class HazardLevel { LOW = 0, MEDIUM = 1, HIGH = 2, SURVIVOR = 3 };

static std::string levelToString(HazardLevel l) {
  switch (l) {
    case HazardLevel::LOW:      return "LOW";
    case HazardLevel::MEDIUM:   return "MEDIUM";
    case HazardLevel::HIGH:     return "HIGH";
    case HazardLevel::SURVIVOR: return "SURVIVOR";
  }
  return "UNKNOWN";
}

class DecisionNode : public rclcpp::Node
{
public:
  DecisionNode() : Node("decision_node")
  {
    this->declare_parameter("robot_id",       "robot_1");
    this->declare_parameter("alert_topic",    "/robot/alerts");

    robot_id_ = this->get_parameter("robot_id").as_string();

    detection_sub_ = this->create_subscription<visualization_msgs::msg::MarkerArray>(
      "/robot/detections", 10,
      std::bind(&DecisionNode::detectionCallback, this, std::placeholders::_1));

    survivor_sub_ = this->create_subscription<geometry_msgs::msg::PoseArray>(
      "/robot/survivor_candidates", 10,
      std::bind(&DecisionNode::survivorCallback, this, std::placeholders::_1));

    alert_pub_ = this->create_publisher<std_msgs::msg::String>(
      this->get_parameter("alert_topic").as_string(), 10);

    RCLCPP_INFO(this->get_logger(), "DecisionNode ready [%s]", robot_id_.c_str());
  }

private:
  void detectionCallback(const visualization_msgs::msg::MarkerArray::SharedPtr msg)
  {
    if (msg->markers.empty()) return;

    // Count fire vs smoke markers to determine hazard level
    int fire_count = 0, smoke_count = 0;
    for (const auto & m : msg->markers) {
      if (m.ns == "fire")  fire_count++;
      if (m.ns == "smoke") smoke_count++;
    }

    HazardLevel level = HazardLevel::LOW;
    if (fire_count > 3)       level = HazardLevel::HIGH;
    else if (fire_count > 0)  level = HazardLevel::MEDIUM;
    else if (smoke_count > 0) level = HazardLevel::LOW;

    publishAlert(level, "fire_detection",
      {{"fire_count", fire_count}, {"smoke_count", smoke_count}});
  }

  void survivorCallback(const geometry_msgs::msg::PoseArray::SharedPtr msg)
  {
    if (msg->poses.empty()) return;

    for (const auto & pose : msg->poses) {
      json extra = {
        {"x", pose.position.x},
        {"y", pose.position.y},
        {"z", pose.position.z}
      };
      publishAlert(HazardLevel::SURVIVOR, "survivor_detected", extra);
    }
  }

  void publishAlert(HazardLevel level, const std::string & type, const json & extra)
  {
    auto now = this->now();
    json alert = {
      {"robot_id",   robot_id_},
      {"timestamp",  now.seconds()},
      {"type",       type},
      {"level",      levelToString(level)},
      {"data",       extra}
    };

    std_msgs::msg::String msg;
    msg.data = alert.dump();
    alert_pub_->publish(msg);

    RCLCPP_WARN(this->get_logger(), "[%s] %s — %s",
      levelToString(level).c_str(), type.c_str(), extra.dump().c_str());
  }

  std::string robot_id_;
  rclcpp::Subscription<visualization_msgs::msg::MarkerArray>::SharedPtr detection_sub_;
  rclcpp::Subscription<geometry_msgs::msg::PoseArray>::SharedPtr         survivor_sub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr                    alert_pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DecisionNode>());
  rclcpp::shutdown();
  return 0;
}
