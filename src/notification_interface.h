#ifndef STORAGE_BENCH_NOTIFICATION_INTERFACE_H
#define STORAGE_BENCH_NOTIFICATION_INTERFACE_H

#include <vector>
#include "storage_interface.h"

class notification_interface {
 public:
  typedef boost::property_tree::ptree property_map;

  virtual void init(const property_map &conf, bool create) = 0;

  virtual void subscribe(const std::string& channel) = 0;

  virtual void publish(const std::string &channel, const std::string &msg) = 0;

  virtual std::vector<std::vector<uint64_t>> get_latencies() const = 0;

  virtual void destroy() = 0;

  virtual void wait(size_t i) = 0;

 protected:
  std::vector<uint64_t> m_publish_ts;
  std::vector<std::vector<uint64_t>> m_notification_ts;
};

#endif //STORAGE_BENCH_NOTIFICATION_INTERFACE_H
