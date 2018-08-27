#include "rate_limiter.h"

#include <thread>

rate_limiter::rate_limiter() : m_interval(0), m_max_permits(0), m_stored_permits(0), m_next_free(0) {}

rate_limiter::rate_limiter(double rate) : m_interval(0), m_max_permits(0), m_stored_permits(0), m_next_free(0) {
  set_rate(rate);
}

int64_t rate_limiter::acquire() {
  auto wait_time = claim_next(1);
  std::this_thread::sleep_for(wait_time);
  return static_cast<int64_t>(wait_time.count() / 1000.0);
}

void rate_limiter::set_rate(double rate) {
  if (rate <= 0.0) {
    throw std::runtime_error("RateLimiter: Rate must be greater than 0");
  }

  std::lock_guard<std::mutex> lock(m_mtx);
  m_interval = 1000000.0 / rate;
}

double rate_limiter::get_rate() const {
  return 1000000.0 / m_interval;
}

void rate_limiter::sync(uint64_t now) {
  // If we're passed the next_free, then recalculate
  // stored permits, and update next_free_
  if (now > m_next_free) {
    m_stored_permits = std::min(m_max_permits, m_stored_permits + (now - m_next_free) / m_interval);
    m_next_free = now;
  }
}

std::chrono::microseconds rate_limiter::claim_next(double permits) {
  using namespace std::chrono;

  std::lock_guard<std::mutex> lock(m_mtx);

  uint64_t now = static_cast<uint64_t>(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());

  // Make sure we're synced
  sync(now);

  // Since we synced before hand, this will always be >= 0.
  unsigned long long wait = m_next_free - now;

  // Determine how many stored and freh permits to consume
  double stored = std::min(permits, m_stored_permits);
  double fresh = permits - stored;

  // In the general RateLimiter, stored permits have no wait time,
  // and thus we only have to wait for however many fresh permits we consume
  long next_free = (long) (fresh * m_interval);

  m_next_free += next_free;
  m_stored_permits -= stored;
  return microseconds(wait);
}

