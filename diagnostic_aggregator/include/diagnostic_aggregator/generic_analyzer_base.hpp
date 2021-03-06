// Copyright 2015 Open Source Robotics Foundation, Inc.
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
#ifndef DIAGNOSTIC_AGGREGATOR__GENERIC_ANALYZER_BASE_HPP_
#define DIAGNOSTIC_AGGREGATOR__GENERIC_ANALYZER_BASE_HPP_

#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "pluginlib/class_list_macros.hpp"
#include "diagnostic_aggregator/analyzer.hpp"
#include "diagnostic_aggregator/status_item.hpp"
#include "diagnostic_msgs/msg/diagnostic_status.hpp"
#include "diagnostic_msgs/msg/key_value.hpp"
#include "rclcpp/rclcpp.hpp"
// TODO(tfoote replace these terrible macros)
#define ROS_ERROR printf
#define ROS_FATAL printf
#define ROS_WARN printf
#define ROS_INFO printf

namespace diagnostic_aggregator
{

/*!
 *\brief GenericAnalyzerBase is the base class for GenericAnalyzer and
 *OtherAnalyzer
 *
 * GenericAnalyzerBase contains the getPath(), getName(), analyze() and report()
 *functions of the Generic and Other Analyzers. It is a virtual class, and
 *cannot be instantiated or loaded as a plugin. Subclasses are responsible for
 *implementing the init() and match() functions.
 *
 * The GenericAnalyzerBase holds the state of the analyzer, and tracks if items
 *are stale, and if the user has the correct number of items.
 */
class GenericAnalyzerBase : public Analyzer
{
public:
  GenericAnalyzerBase()
  : nice_name_(""), path_(""), timeout_(-1.0), num_items_expected_(-1),
    discard_stale_(false), has_initialized_(false), has_warned_(false) {}

  virtual ~GenericAnalyzerBase() {items_.clear();}

  /*
   *\brief Cannot be initialized from (string, NodeHandle) like defined
   *Analyzers
   */
  // bool init(const std::string path, const rclcpp::Node::SharedPtr & n) = 0;
  bool init(
    const std::string base_path, const char *,
    const rclcpp::Node::SharedPtr & n, const char *) = 0;

  /*
   *\brief Must be initialized with path, and a "nice name"
   *
   * Must be initialized in order to prepend the path to all outgoing status
   *messages.
   */
  bool init_v(
    const std::string path, const std::string nice_name,
    double timeout = -1.0, int num_items_expected = -1,
    bool discard_stale = false)
  {
    num_items_expected_ = num_items_expected;
    timeout_ = timeout;
    nice_name_ = nice_name;
    path_ = path;
    discard_stale_ = discard_stale;

    if (discard_stale_ && timeout <= 0) {
      ROS_WARN("Cannot discard stale items if no timeout specified. No items "
        "will be discarded");
      discard_stale_ = false;
    }

    has_initialized_ = true;

    return true;
  }

  /*!
   *\brief Update state with new StatusItem
   */
  virtual bool analyze(const std::shared_ptr<StatusItem> item)
  {
    if (!has_initialized_ && !has_warned_) {
      has_warned_ = true;
      ROS_ERROR("GenericAnalyzerBase is asked to analyze diagnostics without "
        "being initialized. init() must be called in order to "
        "correctly use this class.");
    }

    if (!has_initialized_) {
      return false;
    }

    items_[item->getName()] = item;

    return has_initialized_;
  }

  /*!
   *\brief Reports current state, returns vector of formatted status messages
   *
   *\return Vector of DiagnosticStatus messages. They must have the correct
   *prefix for all names.
   */
  virtual std::vector<std::shared_ptr<diagnostic_msgs::msg::DiagnosticStatus>>
  report()
  {
    if (!has_initialized_ && !has_warned_) {
      has_warned_ = true;
      ROS_ERROR("GenericAnalyzerBase is asked to report diagnostics without "
        "being initialized. init() must be called in order to "
        "correctly use this class.");
    }
    if (!has_initialized_) {
      std::vector<std::shared_ptr<diagnostic_msgs::msg::DiagnosticStatus>> vec;
      return vec;
    }

    std::shared_ptr<diagnostic_msgs::msg::DiagnosticStatus> header_status(
      new diagnostic_msgs::msg::DiagnosticStatus());
    header_status->name = path_;
    header_status->level = 0;
    header_status->message = "OK";

    std::vector<std::shared_ptr<diagnostic_msgs::msg::DiagnosticStatus>>
    processed;
    processed.push_back(header_status);

    bool all_stale = true;

    std::map<std::string, std::shared_ptr<StatusItem>>::iterator it =
      items_.begin();
    while (it != items_.end()) {
      std::string name = it->first;
      std::shared_ptr<StatusItem> item = it->second;

      bool stale = false;
      if (timeout_ > 0) {
        rclcpp::Clock ros_clock(RCL_ROS_TIME);
        rclcpp::Time update_time_now1_ = ros_clock.now();
        stale =
          (((update_time_now1_ - item->getLastUpdateTime()).nanoseconds()) *
          1e-9) > timeout_;
      }

      // Erase item if its stale and we're discarding items
      if (discard_stale_ && stale) {
        items_.erase(it++);
        continue;
      }

      uint8_t level = item->getLevel();
      header_status->level = std::max(header_status->level, level);

      diagnostic_msgs::msg::KeyValue kv;
      kv.key = name;
      kv.value = item->getMessage();

      header_status->values.push_back(kv);

      all_stale = all_stale && ((level == 3) || stale);

      // boost::shared_ptr<diagnostic_msgs::DiagnosticStatus> stat =
      // item->toStatusMsg(path_, stale);

      processed.push_back(item->toStatusMsg(path_, stale));

      if (stale) {
        header_status->level = 3;
      }

      ++it;
    }

    // Header is not stale unless all subs are
    if (all_stale) {
      header_status->level = 3;
    } else if (header_status->level == 3) {
      header_status->level = 2;
    }

    header_status->message = valToMsg(header_status->level);

    // If we expect a given number of items, check that we have this number
    if (num_items_expected_ == 0 && items_.size() == 0) {
      header_status->level = 0;
      header_status->message = "OK";
    } else {
      if (num_items_expected_ > 0 &&
        static_cast<int>(items_.size()) != num_items_expected_)
      {
        uint8_t lvl = 2;
        header_status->level = std::max(lvl, header_status->level);

        std::stringstream expec, item;
        expec << num_items_expected_;
        item << items_.size();

        if (items_.size() > 0) {
          header_status->message =
            "Expected " + expec.str() + ", found " + item.str();
        } else {
          header_status->message = "No items found, expected " + expec.str();
        }
      }
    }

    return processed;
  }

  /*!
   *\brief Match function isn't implemented by GenericAnalyzerBase
   */
  virtual bool match(const std::string name) = 0;

  /*!
   *\brief Returns full prefix (ex: "/Robot/Power System")
   */
  virtual std::string getPath() const {return path_;}

  /*!
   *\brief Returns nice name (ex: "Power System")
   */
  virtual std::string getName() const {return nice_name_;}

protected:
  std::string nice_name_;
  std::string path_;

  double timeout_;
  int num_items_expected_;

  /*!
   *\brief Subclasses can add items to analyze
   */
  void addItem(std::string name, std::shared_ptr<StatusItem> item)
  {
    items_[name] = item;
  }

private:
  /*!
   *\brief Stores items by name. State of analyzer
   */
  std::map<std::string, std::shared_ptr<StatusItem>> items_;

  bool discard_stale_, has_initialized_, has_warned_;
};
}  // namespace diagnostic_aggregator
#endif  // DIAGNOSTIC_AGGREGATOR__GENERIC_ANALYZER_BASE_HPP_
