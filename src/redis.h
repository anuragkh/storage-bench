#ifndef STORAGE_BENCH_REDIS_H
#define STORAGE_BENCH_REDIS_H

#include <queue>
#include "storage_interface.h"
#include "queue.h"
#include <cpp_redis/cpp_redis>

class redis : public storage_interface {
 public:
  void init(const property_map &conf, bool create) override;
  void write(const std::string &key, const std::string &value) override;
  std::string read(const std::string &key) override;
  void destroy() override;
  void write_async(const std::string &key, const std::string &value) override;
  void read_async(const std::string &key) override;
  void wait_write() override;
  std::string wait_read() override;

 private:
  std::future<cpp_redis::reply> send_write(const std::string &key, const std::string &value);
  std::future<cpp_redis::reply> send_read(const std::string &key);

  std::string  parse_read_response(const cpp_redis::reply &r);
  void parse_write_response(const cpp_redis::reply &r);

  static int32_t hash(const std::string& key) {
    return crc16(key.c_str(), key.length());
  }

  static uint16_t crc16(const char *buf, size_t len) {
    size_t counter;
    uint16_t crc = 0;
    for (counter = 0; counter < len; counter++)
      crc = (crc << 8) ^ crc16tab[((crc >> 8) ^ *buf++) & 0x00FF];
    return crc;
  }

  static const uint16_t crc16tab[256];

  std::vector<std::shared_ptr<cpp_redis::client>> m_client;

  queue<std::future<cpp_redis::reply>> m_get_futures;
  queue<std::future<cpp_redis::reply>> m_put_futures;
};

#endif //STORAGE_BENCH_REDIS_H
