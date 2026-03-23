#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <geometry_msgs/msg/pose_array.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include "firefighter_robot/json.hpp"
#include <chrono>
#include <string>
#include <vector>
#include <cmath>

using json = nlohmann::json;

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

struct KnownSurvivor {
  float x, y;
  rclcpp::Time last_reported;
};

class DecisionNode : public rclcpp::Node
{
public:
  DecisionNode() : Node("decision_node")
  {
    this->declare_parameter("robot_id",              "robot_1");
    this->declare_parameter("alert_topic",           "/robot/alerts");
    this->declare_parameter("survivor_cooldown_sec", 30.0);  // re-report after 30s
    this->declare_parameter("survivor_merge_dist",   1.0);   // same survivor if within 1m

    robot_id_         = this->get_parameter("robot_id").as_string();
    survivor_cooldown_ = this->get_parameter("survivor_cooldown_sec").as_double();
    survivor_merge_   = this->get_parameter("survivor_merge_dist").as_double();

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
    auto now = this->now();

    for (const auto & pose : msg->poses) {
      float x = pose.position.x;
      float y = pose.position.y;

      // Check if this is a known survivor reported recently
      bool suppress = false;
      for (auto & known : known_survivors_) {
        float dx = known.x - x;
        float dy = known.y - y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < survivor_merge_) {
          double elapsed = (now - known.last_reported).seconds();
          if (elapsed < survivor_cooldown_) {
            suppress = true;
          } else {
            // Same location but cooldown expired — re-report and update time
            known.last_reported = now;
          }
          break;
        }
      }

      if (suppress) continue;

      // New survivor location — add to known list
      known_survivors_.push_back({x, y, now});

      json extra = {{"x", x}, {"y", y}, {"z", pose.position.z}};
      publishAlert(HazardLevel::SURVIVOR, "survivor_detected", extra);
    }
  }

  void publishAlert(HazardLevel level, const std::string & type, const json & extra)
  {
    auto now = this->now();
    json alert = {
      {"robot_id",  robot_id_},
      {"timestamp", now.seconds()},
      {"type",      type},
      {"level",     levelToString(level)},
      {"data",      extra}
    };

    std_msgs::msg::String msg;
    msg.data = alert.dump();
    alert_pub_->publish(msg);

    RCLCPP_WARN(this->get_logger(), "[%s] %s — %s",
      levelToString(level).c_str(), type.c_str(), extra.dump().c_str());
  }

  std::string robot_id_;
  double survivor_cooldown_;
  double survivor_merge_;
  std::vector<KnownSurvivor> known_survivors_;

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
