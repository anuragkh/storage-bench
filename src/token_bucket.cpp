#include "token_bucket.h"

token_bucket::token_bucket(uint64_t rate, uint64_t burst_size) {
  m_time_per_token = 1000000 / rate;
  m_time_per_burst = burst_size * m_time_per_token;
}

token_bucket::token_bucket(const token_bucket &other) {
  m_time_per_token = other.m_time_per_token.load();
  m_time_per_burst = other.m_time_per_burst.load();
}

token_bucket &token_bucket::operator=(const token_bucket &other) {
  m_time_per_token = other.m_time_per_token.load();
  m_time_per_burst = other.m_time_per_burst.load();
  return *this;
}

bool token_bucket::consume(uint64_t tokens) {
  using namespace std::chrono;
  auto now = static_cast<uint64_t>(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
  auto time_needed = tokens * m_time_per_token.load(std::memory_order_relaxed);
  auto min_time = now - m_time_per_burst.load(std::memory_order_relaxed);
  auto old_time = m_time.load(std::memory_order_relaxed);
  auto new_time = old_time;

  if (min_time > old_time)
    new_time = min_time;

  for (;;) {
    new_time += time_needed;
    if (new_time > now)
      return false;
    if (m_time.compare_exchange_weak(old_time, new_time, std::memory_order_relaxed, std::memory_order_relaxed))
      return true;
    new_time = old_time;
  }
}
