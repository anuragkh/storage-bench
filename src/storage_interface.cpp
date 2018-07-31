#include <random>
#include "storage_interface.h"

std::shared_ptr<storage_interfaces::interface_map> storage_interfaces::m_interface_map{nullptr};

std::string storage_interface::random_string(size_t length) {
  static auto &charset = "0123456789abcdefghijklmnopqrstuvwxyz";
  thread_local static std::mt19937 rg{std::random_device{}()};
  thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(charset) - 2);
  std::string s;
  s.reserve(length);
  while (length--)
    s += charset[pick(rg)];
  return s;
}
