#include "rate_limiter.h"

#include <thread>

rate_limiter::rate_limiter() : m_bucket() {}

rate_limiter::rate_limiter(uint64_t rate) : m_bucket(rate, BURST_SIZE) {}

bool rate_limiter::acquire() {
  return m_bucket.consume(1);
}