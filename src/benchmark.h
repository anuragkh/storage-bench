#ifndef STORAGE_BENCH_BENCHMARK_H
#define STORAGE_BENCH_BENCHMARK_H

#include <memory>
#include "storage_interface.h"

#ifndef ERROR_MAX
#define ERROR_MAX 1000
#endif

class benchmark {
 public:
  static void run(const std::shared_ptr<storage_interface> &iface,
                  const std::string &output_path,
                  size_t value_size,
                  size_t num_ops,
                  const storage_interface::property_map &conf);

  static uint64_t now_us();
};

#endif //STORAGE_BENCH_BENCHMARK_H
