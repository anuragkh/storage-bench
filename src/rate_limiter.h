#ifndef STORAGE_BENCH_RATE_LIMITER_H
#define STORAGE_BENCH_RATE_LIMITER_H

#include <cstdint>
#include <chrono>
#include <mutex>

class rate_limiter {
 public:
  rate_limiter();
  explicit rate_limiter(double rate);

  int64_t acquire();

  void set_rate(double rate);
  double get_rate() const;
 private:
  void sync(uint64_t now);
  std::chrono::microseconds claim_next(double permits);

 private:
  double m_interval;
  double m_max_permits;
  double m_stored_permits;

  uint64_t m_next_free;
  std::mutex m_mtx;
};

#endif //STORAGE_BENCH_RATE_LIMITER_H
