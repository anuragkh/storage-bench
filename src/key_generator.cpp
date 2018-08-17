
#include <random>
#include "key_generator.h"

std::string sequential_key_generator::next() {
  return std::to_string(cur_key_++);
}

zipf_key_generator::zipf_key_generator(double theta, uint64_t n)
    : theta_(theta), n_(n), zdist_(new double[n]), dist_(0, 1) {
  rng_.seed(std::random_device()());
  gen_zipf();
}

void zipf_key_generator::gen_zipf() {
  double sum = 0.0;
  double c;
  double expo = 1.0 - theta_;
  double sumc = 0.0;
  uint64_t i;

  /*
   * Zipfian - p(i) = c / i ^^ (1 - theta)
   * At theta = 1, uniform
   * At theta = 0, pure zipfian
   */

  for (i = 1; i <= n_; i++) {
    sum += 1.0 / pow((double) i, expo);
  }
  c = 1.0 / sum;

  for (i = 0; i < n_; i++) {
    sumc += c / pow((double) (i + 1), expo);
    zdist_[i] = sumc;
  }
}

zipf_key_generator::~zipf_key_generator() {
  delete[] zdist_;
}

std::string zipf_key_generator::next() {
  double r = dist_(rng_);
  int64_t lo = 0;
  int64_t hi = n_;
  while (lo != hi) {
    int64_t mid = (lo + hi) / 2;
    if (zdist_[mid] <= r) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }

  return std::to_string(lo);
}
