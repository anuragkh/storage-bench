#ifndef STORAGE_BENCH_RATE_LIMITER_H
#define STORAGE_BENCH_RATE_LIMITER_H

#include <cstdint>
#include <chrono>
#include <mutex>
#include "token_bucket.h"

#ifndef BURST_SIZE
#define BURST_SIZE 1000
#endif

class rate_limiter {
 public:
  rate_limiter();
  explicit rate_limiter(uint64_t rate);

  bool acquire();

 private:
  token_bucket m_bucket;
};

#endif //STORAGE_BENCH_RATE_LIMITER_H
