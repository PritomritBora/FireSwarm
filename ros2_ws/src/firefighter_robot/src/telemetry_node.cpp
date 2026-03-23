#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <curl/curl.h>
#include <string>

// Telemetry node — receives alert JSON strings and POSTs them to the backend.
// Uses libcurl for HTTP, keeping everything in C++ without Python bridge overhead.

class TelemetryNode : public rclcpp::Node
{
public:
  TelemetryNode() : Node("telemetry_node")
  {
    this->declare_parameter("backend_url", "http://localhost:8000/alerts");
    this->declare_parameter("alert_topic", "/robot/alerts");

    backend_url_ = this->get_parameter("backend_url").as_string();

    curl_global_init(CURL_GLOBAL_DEFAULT);

    sub_ = this->create_subscription<std_msgs::msg::String>(
      this->get_parameter("alert_topic").as_string(), 10,
      std::bind(&TelemetryNode::alertCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "TelemetryNode → %s", backend_url_.c_str());
  }

  ~TelemetryNode() { curl_global_cleanup(); }

private:
  void alertCallback(const std_msgs::msg::String::SharedPtr msg)
  {
    CURL * curl = curl_easy_init();
    if (!curl) {
      RCLCPP_ERROR(this->get_logger(), "Failed to init curl");
      return;
    }

    struct curl_slist * headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,            backend_url_.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     msg->data.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        5L);   // 5s timeout
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1L);   // thread-safe

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      RCLCPP_WARN(this->get_logger(), "Telemetry POST failed: %s",
        curl_easy_strerror(res));
      // TODO Phase 6: buffer failed messages and replay on reconnect
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }

  std::string backend_url_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<TelemetryNode>());
  rclcpp::shutdown();
  return 0;
}
