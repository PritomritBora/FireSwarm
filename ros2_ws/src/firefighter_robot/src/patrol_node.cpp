#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <chrono>

using namespace std::chrono_literals;

// Simple timed patrol — moves the robot through the building
// without requiring Nav2 or a map. Good enough for demo.

struct PatrolStep {
  double linear;   // m/s
  double angular;  // rad/s
  int    duration; // milliseconds
};

// Patrol pattern covering the building corridors
static const std::vector<PatrolStep> PATROL = {
  {0.2,  0.0,  4000},  // forward 4s
  {0.0,  0.8,  2000},  // turn left ~90deg
  {0.2,  0.0,  3000},  // forward 3s
  {0.0, -0.8,  2000},  // turn right ~90deg
  {0.2,  0.0,  4000},  // forward 4s
  {0.0, -0.8,  2000},  // turn right ~90deg
  {0.2,  0.0,  3000},  // forward 3s
  {0.0,  0.8,  2000},  // turn left ~90deg
  {0.2,  0.0,  4000},  // forward 4s
  {0.0,  0.8,  4000},  // turn around 180deg
  {0.2,  0.0,  8000},  // long forward pass
  {0.0,  0.8,  2000},  // turn left
  {0.2,  0.0,  3000},  // forward
  {0.0,  0.8,  2000},  // turn left
  {0.2,  0.0,  4000},  // forward
};

class PatrolNode : public rclcpp::Node
{
public:
  PatrolNode() : Node("patrol_node"), step_(0), step_start_(this->now())
  {
    this->declare_parameter("cmd_vel_topic", "/cmd_vel");
    this->declare_parameter("patrol_speed",  0.2);

    auto topic = this->get_parameter("cmd_vel_topic").as_string();

    cmd_pub_ = this->create_publisher<geometry_msgs::msg::Twist>(topic, 10);

    // 100ms timer — check if current step is done
    timer_ = this->create_wall_timer(100ms,
      std::bind(&PatrolNode::timerCallback, this));

    RCLCPP_INFO(this->get_logger(),
      "PatrolNode started — %zu steps in patrol pattern", PATROL.size());
  }

private:
  void timerCallback()
  {
    auto now     = this->now();
    auto elapsed = (now - step_start_).nanoseconds() / 1e6;  // ms

    const auto & step = PATROL[step_];

    if (elapsed >= step.duration) {
      // Move to next step
      step_ = (step_ + 1) % PATROL.size();
      step_start_ = now;
      RCLCPP_INFO(this->get_logger(), "Patrol step %zu/%zu",
        step_ + 1, PATROL.size());
    }

    geometry_msgs::msg::Twist cmd;
    cmd.linear.x  = PATROL[step_].linear;
    cmd.angular.z = PATROL[step_].angular;
    cmd_pub_->publish(cmd);
  }

  size_t step_;
  rclcpp::Time step_start_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PatrolNode>());
  rclcpp::shutdown();
  return 0;
}
