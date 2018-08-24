#ifndef STORAGE_BENCH_KEY_GENERATOR_H
#define STORAGE_BENCH_KEY_GENERATOR_H

#include <string>
#include <random>

class sequential_key_generator {
 public:
  sequential_key_generator() = default;

  std::string next();
  void reset();

 private:
  size_t cur_key_{0};
};

class zipf_key_generator {
 public:
  struct probvals {
    double prob;        // The access probability
    double cum_prob;    // The cumulative access probability
  };

  zipf_key_generator(double theta, uint64_t n);
  ~zipf_key_generator();

  std::string next();
  void reset();

 private:
  void gen_zipf();

  double theta_;       // The skew parameter (0=pure zipf, 1=pure uniform)
  uint64_t n_;         // The number of objects
  double *zdist_;

  std::mt19937 rng_;
  std::uniform_real_distribution<> dist_;
};

#endif //STORAGE_BENCH_KEY_GENERATOR_H
