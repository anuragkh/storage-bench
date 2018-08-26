#ifndef STORAGE_BENCH_TOKEN_BUCKET_H
#define STORAGE_BENCH_TOKEN_BUCKET_H

#include <atomic>
#include <chrono>

class token_bucket {
 public:
  token_bucket() = default;

  token_bucket(uint64_t rate, uint64_t burst_size);

  token_bucket(const token_bucket &other);

  token_bucket &operator=(const token_bucket &other);

  bool consume(uint64_t tokens);

 private:
  std::atomic<uint64_t> m_time = {0};
  std::atomic<uint64_t> m_time_per_token = {0};
  std::atomic<uint64_t> m_time_per_burst = {0};
};

#endif //STORAGE_BENCH_TOKEN_BUCKET_H
