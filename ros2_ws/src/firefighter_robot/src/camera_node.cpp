#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

// Camera node — subscribes to raw camera topic and republishes.
// In simulation Gazebo publishes directly to /camera/image_raw.
// This node is a passthrough that can be extended with preprocessing
// (undistortion, exposure correction, etc.) without touching inference_node.

class CameraNode : public rclcpp::Node
{
public:
  CameraNode() : Node("camera_node")
  {
    this->declare_parameter("input_topic",  "/camera/image_raw");
    this->declare_parameter("output_topic", "/robot/camera/image");

    auto in  = this->get_parameter("input_topic").as_string();
    auto out = this->get_parameter("output_topic").as_string();

    sub_ = this->create_subscription<sensor_msgs::msg::Image>(
      in, rclcpp::SensorDataQoS(),
      [this, out](const sensor_msgs::msg::Image::SharedPtr msg) {
        // Passthrough — extend here for preprocessing
        pub_->publish(*msg);
      });

    pub_ = this->create_publisher<sensor_msgs::msg::Image>(out, rclcpp::SensorDataQoS());

    RCLCPP_INFO(this->get_logger(), "CameraNode: %s → %s", in.c_str(), out.c_str());
  }

private:
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr    pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraNode>());
  rclcpp::shutdown();
  return 0;
}
