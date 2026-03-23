#include "firefighter_robot/inference_node.hpp"

#include <sensor_msgs/msg/image.hpp>
#include <visualization_msgs/msg/marker.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include <opencv2/dnn.hpp>
#include <stdexcept>

namespace firefighter_robot
{

// Class labels matching the exported ONNX model
static const std::vector<std::string> CLASS_NAMES = {"fire", "smoke"};

InferenceNode::InferenceNode(const rclcpp::NodeOptions & options)
: Node("inference_node", options),
  env_(ORT_LOGGING_LEVEL_WARNING, "inference_node")
{
  this->declare_parameter("model_path",      "models/fire_smoke.onnx");
  this->declare_parameter("conf_threshold",  0.45f);
  this->declare_parameter("input_width",     640);
  this->declare_parameter("input_height",    640);
  this->declare_parameter("image_topic",     "/robot/camera/image");
  this->declare_parameter("detection_topic", "/robot/detections");

  model_path_     = this->get_parameter("model_path").as_string();
  conf_threshold_ = static_cast<float>(this->get_parameter("conf_threshold").as_double());
  input_width_    = this->get_parameter("input_width").as_int();
  input_height_   = this->get_parameter("input_height").as_int();

  reloadModel(model_path_);

  reentrant_group_ = this->create_callback_group(
    rclcpp::CallbackGroupType::Reentrant);

  rclcpp::SubscriptionOptions sub_options;
  sub_options.callback_group = reentrant_group_;

  auto image_topic     = this->get_parameter("image_topic").as_string();
  auto detection_topic = this->get_parameter("detection_topic").as_string();

  image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
    image_topic, rclcpp::SensorDataQoS(),
    std::bind(&InferenceNode::imageCallback, this, std::placeholders::_1),
    sub_options);

  detection_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
    detection_topic, 10);

  RCLCPP_INFO(this->get_logger(), "InferenceNode ready. Model: %s", model_path_.c_str());
}

void InferenceNode::reloadModel(const std::string & path)
{
  session_options_.SetIntraOpNumThreads(2);
  session_options_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
  session_ = std::make_unique<Ort::Session>(env_, path.c_str(), session_options_);
  RCLCPP_INFO(this->get_logger(), "Model loaded: %s", path.c_str());
}

void InferenceNode::imageCallback(const sensor_msgs::msg::Image::SharedPtr msg)
{
  // Convert ROS image message to OpenCV Mat (assume BGR8 or rgb8)
  cv::Mat frame(msg->height, msg->width, CV_8UC3,
                const_cast<uint8_t *>(msg->data.data()));

  if (msg->encoding == "rgb8") {
    cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
  }

  auto detections = runInference(frame);

  // Publish as MarkerArray so RViz can visualise bounding boxes
  visualization_msgs::msg::MarkerArray marker_array;
  int id = 0;
  for (const auto & det : detections) {
    visualization_msgs::msg::Marker m;
    m.header.stamp    = msg->header.stamp;
    m.header.frame_id = "camera_frame";
    m.ns              = CLASS_NAMES[det.class_id];
    m.id              = id++;
    m.type            = visualization_msgs::msg::Marker::CUBE;
    m.action          = visualization_msgs::msg::Marker::ADD;
    m.scale.x         = 0.1;
    m.scale.y         = 0.1;
    m.scale.z         = 0.1;
    // fire = red, smoke = grey
    m.color.a = 0.8f;
    m.color.r = (det.class_id == 0) ? 1.0f : 0.5f;
    m.color.g = 0.0f;
    m.color.b = (det.class_id == 1) ? 0.5f : 0.0f;
    marker_array.markers.push_back(m);
  }
  detection_pub_->publish(marker_array);
}

cv::Mat InferenceNode::preprocessFrame(const cv::Mat & frame)
{
  cv::Mat resized, blob;
  cv::resize(frame, resized, cv::Size(input_width_, input_height_));
  resized.convertTo(blob, CV_32F, 1.0 / 255.0);
  // HWC → CHW
  cv::dnn::blobFromImage(resized, blob, 1.0 / 255.0,
    cv::Size(input_width_, input_height_), cv::Scalar(), true, false);
  return blob;
}

std::vector<Detection> InferenceNode::runInference(const cv::Mat & frame)
{
  cv::Mat blob = preprocessFrame(frame);

  // Build input tensor
  std::vector<int64_t> input_shape = {1, 3, input_height_, input_width_};
  size_t input_size = 1 * 3 * input_height_ * input_width_;

  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
    memory_info,
    reinterpret_cast<float *>(blob.data),
    input_size,
    input_shape.data(),
    input_shape.size());

  // Get input/output names
  auto input_name_ptr  = session_->GetInputNameAllocated(0, allocator_);
  auto output_name_ptr = session_->GetOutputNameAllocated(0, allocator_);
  const char * input_names[]  = {input_name_ptr.get()};
  const char * output_names[] = {output_name_ptr.get()};

  auto outputs = session_->Run(
    Ort::RunOptions{nullptr},
    input_names, &input_tensor, 1,
    output_names, 1);

  return postprocess(outputs, frame.cols, frame.rows);
}

std::vector<Detection> InferenceNode::postprocess(
  const std::vector<Ort::Value> & outputs,
  int orig_w, int orig_h)
{
  std::vector<Detection> detections;

  // YOLOv8 output shape: [1, num_classes+4, num_anchors]
  auto * data = outputs[0].GetTensorData<float>();
  auto shape  = outputs[0].GetTensorTypeAndShapeInfo().GetShape();

  int num_classes = static_cast<int>(shape[1]) - 4;
  int num_anchors = static_cast<int>(shape[2]);

  float x_scale = static_cast<float>(orig_w) / input_width_;
  float y_scale = static_cast<float>(orig_h) / input_height_;

  for (int i = 0; i < num_anchors; ++i) {
    // Find best class
    float max_conf = 0.0f;
    int   best_cls = 0;
    for (int c = 0; c < num_classes; ++c) {
      float conf = data[(4 + c) * num_anchors + i];
      if (conf > max_conf) { max_conf = conf; best_cls = c; }
    }

    if (max_conf < conf_threshold_) continue;

    float cx = data[0 * num_anchors + i] * x_scale;
    float cy = data[1 * num_anchors + i] * y_scale;
    float w  = data[2 * num_anchors + i] * x_scale;
    float h  = data[3 * num_anchors + i] * y_scale;

    Detection det;
    det.class_id   = best_cls;
    det.confidence = max_conf;
    det.bbox       = cv::Rect(
      static_cast<int>(cx - w / 2), static_cast<int>(cy - h / 2),
      static_cast<int>(w),          static_cast<int>(h));
    detections.push_back(det);
  }

  return detections;
}

}  // namespace firefighter_robot

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<firefighter_robot::InferenceNode>();
  // Multithreaded so new frames can be queued while inference is running
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
  return 0;
}
