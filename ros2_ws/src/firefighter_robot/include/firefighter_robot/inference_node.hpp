#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

#include <string>
#include <vector>
#include <memory>

namespace firefighter_robot
{

struct Detection {
  int class_id;       // 0=fire, 1=smoke
  float confidence;
  cv::Rect bbox;
};

class InferenceNode : public rclcpp::Node
{
public:
  explicit InferenceNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg);
  std::vector<Detection> runInference(const cv::Mat & frame);
  cv::Mat preprocessFrame(const cv::Mat & frame);
  std::vector<Detection> postprocess(
    const std::vector<Ort::Value> & outputs,
    int orig_w, int orig_h);
  void reloadModel(const std::string & model_path);

  // ONNX Runtime
  Ort::Env env_;
  Ort::SessionOptions session_options_;
  std::unique_ptr<Ort::Session> session_;
  Ort::AllocatorWithDefaultOptions allocator_;

  rclcpp::CallbackGroup::SharedPtr reentrant_group_;

  // Node params
  std::string model_path_;
  float conf_threshold_;
  int input_width_;
  int input_height_;

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr detection_pub_;
};

}  // namespace firefighter_robot
