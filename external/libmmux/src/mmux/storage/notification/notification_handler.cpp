#include <iostream>
#include "notification_handler.h"

namespace mmux {
namespace storage {

notification_handler::notification_handler(mailbox_t &notifications,
                                           mailbox_t &controls)
    : notifications_(notifications), controls_(controls) {}

void notification_handler::notification(const std::string &op, const std::string &data) {
  notifications_.push(std::make_pair(op, data));
}

void notification_handler::control(response_type type, const std::vector<std::string> &ops, const std::string &error) {
  std::string msg = type == response_type::subscribe ? "Subscribe: " : "Unsubscribe: ";
  if (error == "") {
    for (const auto &op: ops)
      msg += op + ", ";
    msg.pop_back();
    msg.pop_back();
    controls_.push(std::make_pair("success", msg));
  } else {
    controls_.push(std::make_pair("error", msg + error));
  }
}

}
}
