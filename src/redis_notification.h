#ifndef STORAGE_BENCH_REDIS_NOTIFICATION_H
#define STORAGE_BENCH_REDIS_NOTIFICATION_H

#include <cpp_redis/cpp_redis>
#include <memory>
#include "notification_interface.h"

class redis_notification : public notification_interface {
 public:

  void init(const property_map &conf, bool create, size_t num_listeners) override;

  void subscribe(const std::string &channel) override;

  void publish(const std::string &channel, const std::string &msg) override;

  void destroy() override;

  void wait(size_t i) override;

 private:
  std::atomic<size_t>* m_sub_msgs{nullptr};
  size_t m_num_listeners{0};
  size_t m_sub_count{0};
  std::vector<std::shared_ptr<cpp_redis::subscriber>> m_sub{};
  std::shared_ptr<cpp_redis::client> m_pub{};
  std::string m_host;
  size_t m_port;
};

#endif //STORAGE_BENCH_REDIS_NOTIFICATION_H
