#ifndef STORAGE_BENCH_REDIS_H
#define STORAGE_BENCH_REDIS_H

#include "storage_interface.h"
#include <cpp_redis/cpp_redis>

class redis: public storage_interface {
 public:
  void init(const property_map &conf) override;
  void write(const std::string &key, const std::string &value) override;
  std::string read(const std::string &key) override;
  void destroy() override;

 private:
  std::shared_ptr<cpp_redis::client> m_client;
};

#endif //STORAGE_BENCH_REDIS_H
