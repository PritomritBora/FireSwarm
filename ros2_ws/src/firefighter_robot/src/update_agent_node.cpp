#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <curl/curl.h>
#include "firefighter_robot/json.hpp"

#include <fstream>
#include <string>
#include <chrono>

using json = nlohmann::json;

// Callback to write curl response body into a string
static size_t writeCallback(char * ptr, size_t size, size_t nmemb, std::string * data)
{
  data->append(ptr, size * nmemb);
  return size * nmemb;
}

// Callback to write downloaded model bytes to a file
static size_t writeFileCallback(char * ptr, size_t size, size_t nmemb, std::ofstream * file)
{
  file->write(ptr, size * nmemb);
  return size * nmemb;
}

class UpdateAgentNode : public rclcpp::Node
{
public:
  UpdateAgentNode() : Node("update_agent_node")
  {
    this->declare_parameter("registry_url",   "http://localhost:8000/model/latest");
    this->declare_parameter("model_path",     "models/fire_smoke.onnx");
    this->declare_parameter("poll_interval",  60);  // seconds
    this->declare_parameter("reload_topic",   "/robot/reload_model");

    registry_url_  = this->get_parameter("registry_url").as_string();
    model_path_    = this->get_parameter("model_path").as_string();
    poll_interval_ = this->get_parameter("poll_interval").as_int();

    reload_pub_ = this->create_publisher<std_msgs::msg::String>(
      this->get_parameter("reload_topic").as_string(), 10);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    timer_ = this->create_wall_timer(
      std::chrono::seconds(poll_interval_),
      std::bind(&UpdateAgentNode::checkForUpdate, this));

    RCLCPP_INFO(this->get_logger(),
      "UpdateAgentNode polling %s every %ds", registry_url_.c_str(), poll_interval_);
  }

  ~UpdateAgentNode() { curl_global_cleanup(); }

private:
  void checkForUpdate()
  {
    std::string response;
    CURL * curl = curl_easy_init();
    if (!curl) return;

    curl_easy_setopt(curl, CURLOPT_URL,           registry_url_.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      RCLCPP_WARN(this->get_logger(), "Registry poll failed: %s", curl_easy_strerror(res));
      return;
    }

    try {
      auto info = json::parse(response);
      std::string version     = info["version"].get<std::string>();
      std::string download_url = info["url"].get<std::string>();

      if (version == current_version_) return;  // already up to date

      RCLCPP_INFO(this->get_logger(),
        "New model version: %s → %s", current_version_.c_str(), version.c_str());

      if (downloadModel(download_url)) {
        current_version_ = version;
        // Signal inference_node to reload
        std_msgs::msg::String msg;
        msg.data = model_path_;
        reload_pub_->publish(msg);
        RCLCPP_INFO(this->get_logger(), "Model updated to %s", version.c_str());
      }
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "Failed to parse registry response: %s", e.what());
    }
  }

  bool downloadModel(const std::string & url)
  {
    std::ofstream file(model_path_, std::ios::binary);
    if (!file.is_open()) {
      RCLCPP_ERROR(this->get_logger(), "Cannot open model path for writing: %s",
        model_path_.c_str());
      return false;
    }

    CURL * curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    file.close();

    return res == CURLE_OK;
  }

  std::string registry_url_;
  std::string model_path_;
  std::string current_version_ = "none";
  int         poll_interval_;

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr reload_pub_;
  rclcpp::TimerBase::SharedPtr                        timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<UpdateAgentNode>());
  rclcpp::shutdown();
  return 0;
}
