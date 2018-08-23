#ifndef STORAGE_BENCH_REDIS_H
#define STORAGE_BENCH_REDIS_H

#include "storage_interface.h"
#include <cpp_redis/cpp_redis>

class redis : public storage_interface {
 public:
  void init(const property_map &conf) override;
  void write(const std::string &key, const std::string &value) override;
  std::string read(const std::string &key) override;
  void destroy() override;
  void write_async(const std::string &key, const std::string &value) override;
  void read_async(const std::string &key) override;
  void wait_writes() override;
  void wait_reads(std::vector<std::string> &results) override;

 private:
  std::future<cpp_redis::reply> send_write(const std::string &key, const std::string &value);
  std::future<cpp_redis::reply> send_read(const std::string &key);

  std::string  parse_read_response(const cpp_redis::reply &r);
  void parse_write_response(const cpp_redis::reply &r);

  std::shared_ptr<cpp_redis::client> m_client;

  std::vector<std::future<cpp_redis::reply>> m_get_futures;
  std::vector<std::future<cpp_redis::reply>> m_put_futures;
};

#endif //STORAGE_BENCH_REDIS_H
