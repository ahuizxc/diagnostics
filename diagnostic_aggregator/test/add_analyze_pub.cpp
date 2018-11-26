// Copyright 2014 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "rcutils/cmdline_parser.h"

#include "std_msgs/msg/string.hpp"
#include "diagnostic_msgs/msg/diagnostic_array.hpp"
#include "diagnostic_msgs/msg/diagnostic_status.hpp"
#include "diagnostic_msgs/msg/key_value.hpp"
#include "diagnostic_msgs/srv/add_diagnostics.hpp"

#include "rclcpp/clock.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/time_source.hpp"

using namespace std;
using namespace std::chrono_literals;

void print_usage()
{
  printf("Usage for talker app:\n");
  printf("talker [-t topic_name] [-h]\n");
  printf("options:\n");
  printf("-h : Print this help function.\n");
  printf("-t topic_name : Specify the topic on which to publish. Defaults to chatter.\n");
}

// Create a AddAnalyzerPub class that subclasses the generic rclcpp::Node base class.
// The main function below will instantiate the class as a ROS node.
class AddAnalyzerPub : public rclcpp::Node
{
public:
  explicit AddAnalyzerPub(const std::string & topic_name)
  : Node("test_add_analyzer")
  {
   // msg_ = std::make_shared<std_msgs::msg::String>();
    msg_ = std::make_shared<diagnostic_msgs::msg::DiagnosticArray>();
    msg_s = std::make_shared<diagnostic_msgs::msg::DiagnosticStatus>();
    
    rclcpp::Clock::SharedPtr clock = std::make_shared<rclcpp::Clock>(RCL_ROS_TIME);
    msg_->header.stamp = clock->now();
    msg_s->name="vebs";
    diagnostic_msgs::msg::DiagnosticStatus msg1,msg2,msg3,msg4,msg5,msg6,msg7,msg8,msg9;
    msg1.name="primary";
    msg1.level=0;
    msg1.message="OK";

    msg2.name="secondary";
    msg2.level=1;
    msg2.message="Warning";


    msg3.name="secondary";
    msg3.level=0;
    msg3.message="OK";

    msg4.name="prefix1: contains1b";
    msg4.level=0;
    msg4.message="OK";

    msg5.name="name1";
    msg5.level=0;
    msg5.message="OK";

    msg6.name="prefix1: expected1a";
    msg6.level=0;
    msg6.message="OK";

    msg7.name="find1_items: find_remove1a";
    msg7.level=0;
    msg7.message="OK";

    msg8.name="contain2a";
    msg8.level=0;
    msg8.message="OK";

    msg9.name="other1";
    msg9.level=2;
    msg9.message="Error";
    vector<diagnostic_msgs::msg::DiagnosticStatus> v_msg;
    v_msg.push_back(msg1);
    v_msg.push_back(msg2);
    v_msg.push_back(msg3);
    v_msg.push_back(msg4);
    v_msg.push_back(msg5);
    v_msg.push_back(msg6);
    v_msg.push_back(msg7);
    v_msg.push_back(msg8);
    v_msg.push_back(msg9);

    msg_->status = v_msg;

        // Create a function for when messages are to be sent.
    auto publish_message =
      [this]() -> void
      {
//msg_->name = "Hello World: " + std::to_string(count_++);
        //RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", msg_->status)
        RCLCPP_INFO(this->get_logger(), "Publishing: namespce %s and name is %s ",this->get_namespace(), this->get_name())

        // Put the message into a queue to be processed by the middleware.
        // This call is non-blocking.
        pub_->publish(msg_);
      };

    // Create a publisher with a custom Quality of Service profile.
    rmw_qos_profile_t custom_qos_profile = rmw_qos_profile_default;
    custom_qos_profile.depth = 7;
    pub_ = this->create_publisher<diagnostic_msgs::msg::DiagnosticArray>("/diagnostics", custom_qos_profile);

    // Use a timer to schedule periodic message publishing.
    timer_ = this->create_wall_timer(1s, publish_message);

    	

  }

private:
  size_t count_ = 1;
  std::shared_ptr<diagnostic_msgs::msg::DiagnosticArray> msg_;
  std::shared_ptr<diagnostic_msgs::msg::DiagnosticStatus> msg_s;
  rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr pub_;
//rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

diagnostic_msgs::srv::AddDiagnostics::Response::SharedPtr send_request(
  rclcpp::Node::SharedPtr node,
  rclcpp::Client<diagnostic_msgs::srv::AddDiagnostics>::SharedPtr client,
  diagnostic_msgs::srv::AddDiagnostics::Request::SharedPtr request)
{
  auto result = client->async_send_request(request);
  // Wait for the result.
  if (rclcpp::spin_until_future_complete(node, result) ==
    rclcpp::executor::FutureReturnCode::SUCCESS)
  {
    return result.get();
  } else {
    return NULL;
  }
}


int main(int argc, char * argv[])
{
  // Force flush of the stdout buffer.
  // This ensures a correct sync of all prints
  // even when executed simultaneously within the launch file.
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  if (rcutils_cli_option_exist(argv, argv + argc, "-h")) {
    print_usage();
    return 0;
  }

  // Initialize any global resources needed by the middleware and the client library.
  // You must call this before using any other part of the ROS system.
  // This should be called once per process.
  rclcpp::init(argc, argv);

  // Create a node.
  
   auto topic = std::string("/diagnostics_agg/add_diagnostics");
  char * cli_option = rcutils_cli_get_option(argv, argv + argc, "-s");
  if (nullptr != cli_option) {
    topic = std::string(cli_option);
  }

  auto node = std::make_shared<AddAnalyzerPub>(topic);
  auto client = node->create_client<diagnostic_msgs::srv::AddDiagnostics>(topic);

  auto request = std::make_shared<diagnostic_msgs::srv::AddDiagnostics::Request>();
	request->load_namespace="/test_add_analyzer";
  while (!client->wait_for_service(3s)) {
    if (!rclcpp::ok()) {
      RCLCPP_ERROR(node->get_logger(), "Interrupted while waiting for the service. Exiting.");
      return 0;
    }
    RCLCPP_INFO(node->get_logger(), "service not available, waiting again...");
  }

  // TODO(wjwwood): make it like `client->send_request(node, request)->sum`
  // TODO(wjwwood): consider error condition
  auto result = send_request(node, client, request);
  if (result) {
    RCLCPP_INFO(node->get_logger(), "Result of add_two_ints: %s", result->message);
  } else {
    RCLCPP_ERROR(node->get_logger(), "Interrupted while waiting for response. Exiting.");
  }



  // spin will block until work comes in, execute work as it becomes available, and keep blocking.
  // It will only be interrupted by Ctrl-C.
  rclcpp::spin(node);

  rclcpp::shutdown();
  return 0;
}