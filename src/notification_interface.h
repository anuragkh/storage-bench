#ifndef STORAGE_BENCH_NOTIFICATION_INTERFACE_H
#define STORAGE_BENCH_NOTIFICATION_INTERFACE_H

#include <vector>
#include "storage_interface.h"

class notification_interface {
 public:
  typedef boost::property_tree::ptree property_map;

  virtual void init(const property_map &conf, bool create) = 0;

  virtual void subscribe(const std::string &channel) = 0;

  virtual void publish(const std::string &channel, const std::string &msg) = 0;

  std::vector<uint64_t> get_publish_ts() const {
    return m_publish_ts;
  }

  std::vector<std::vector<uint64_t>> get_notification_ts() const {
    return m_notification_ts;
  }

  std::vector<std::vector<uint64_t>> get_latencies() const {
    std::vector<std::vector<uint64_t>> ts(m_notification_ts.size());
    for (size_t i = 0; i < m_notification_ts.size(); ++i) {
      for (size_t j = 0; j < m_notification_ts[i].size(); ++j) {
        ts[i].push_back(m_notification_ts[i][j] - m_publish_ts[j]);
      }
    }
    return ts;
  }

  virtual void destroy() = 0;

  virtual void wait(size_t i) = 0;

 protected:
  std::vector<uint64_t> m_publish_ts;
  std::vector<std::vector<uint64_t>> m_notification_ts;
};

class notification_interfaces {
 public:
  typedef std::map<std::string, std::shared_ptr<notification_interface>> interface_map;

  static void register_interface(const std::string &name, std::shared_ptr<notification_interface> iface) {
    interfaces()->insert({name, iface});
  }

  static void deregister_interface(const std::string &name) {
    interfaces()->erase(name);
  }

  static std::shared_ptr<notification_interface> get_interface(const std::string &name) {
    auto it = interfaces()->find(name);
    if (it == interfaces()->end()) {
      throw std::invalid_argument("No such interface " + name);
    }
    return it->second;
  }

  static std::shared_ptr<interface_map> interfaces() {
    if (m_interface_map == nullptr) {
      m_interface_map = std::make_shared<interface_map>();
    }
    return m_interface_map;
  };

 private:
  static std::shared_ptr<interface_map> m_interface_map;
};

#define REGISTER_NOTIFICATION_IFACE(name, iface)                                      \
  class iface##_notification_class {                                                  \
   public:                                                                            \
    iface##_notification_class() {                                                    \
      notification_interfaces::register_interface(name, std::make_shared<iface>());   \
    }                                                                                 \
    ~iface##_notification_class() {                                                   \
      notification_interfaces::deregister_interface(name);                            \
    }                                                                                 \
  };                                                                                  \
  static iface##_notification_class iface##_notification_singleton

#endif //STORAGE_BENCH_NOTIFICATION_INTERFACE_H
