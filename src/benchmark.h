#ifndef STORAGE_BENCH_BENCHMARK_H
#define STORAGE_BENCH_BENCHMARK_H

#include <cstdlib>
#include <chrono>
#include <fstream>
#include <memory>
#include "storage_interface.h"

#ifndef ERROR_MAX
#define ERROR_MAX 1000
#endif

#define BENCHMARK_READ    1
#define BENCHMARK_WRITE   2
#define BENCHMARK_DESTROY 4

class benchmark {
 public:
  template<typename K>
  static void run(const std::shared_ptr<storage_interface> &s_if,
                  const storage_interface::property_map &conf,
                  std::shared_ptr<K> key_gen,
                  const std::string &output_path,
                  size_t value_size,
                  size_t num_ops,
                  bool warm_up,
                  int32_t mode,
                  uint64_t max_us) {
    int err_count = 0;
    size_t warm_up_ops = num_ops / 10;
    std::string value(value_size, 'x');

    auto start_us = now_us();

    std::cerr << "Initializing storage interface..." << std::endl;
    s_if->init(conf);

    if ((mode & BENCHMARK_WRITE) == BENCHMARK_WRITE) {
      std::ofstream lw(output_path + "_write_latency.txt");
      std::ofstream tw(output_path + "_write_throughput.txt");

      if (warm_up) {
        std::cerr << "Warm-up writes..." << std::endl;
        for (size_t i = 0; i < warm_up_ops && time_bound(start_us, max_us); i++) {
          try {
            s_if->write(key_gen->next(), value);
          } catch (std::runtime_error &e) {
            --i;
            ++err_count;
            if (err_count > ERROR_MAX) {
              std::cerr << "Too many errors" << std::endl;
              s_if->destroy();
              std::cerr << "Destroyed storage interface." << std::endl;
              exit(1);
            }
          }
        }
      }

      std::cerr << "Starting writes..." << std::endl;
      auto w_begin = now_us();
      for (size_t i = 0; i < num_ops && time_bound(start_us, max_us); ++i) {
        auto t_b = now_us();
        try {
          s_if->write(key_gen->next(), value);
        } catch (std::runtime_error &e) {
          --i;
          ++err_count;
          if (err_count > ERROR_MAX) {
            std::cerr << "Too many errors" << std::endl;
            s_if->destroy();
            std::cerr << "Destroyed storage interface." << std::endl;
            exit(1);
          }
        }
        auto t_e = now_us();
        lw << (t_e - t_b) << std::endl;
      }
      auto w_end = now_us();
      auto w_elapsed_s = static_cast<double>(w_end - w_begin) / 1000000.0;
      std::cerr << "Finished writes." << std::endl;

      tw << (static_cast<double>(num_ops) / w_elapsed_s) << std::endl;
      lw.close();
      tw.close();
    }

    if ((mode & BENCHMARK_READ) == BENCHMARK_READ) {
      std::ofstream lr(output_path + "_read_latency.txt");
      std::ofstream tr(output_path + "_read_throughput.txt");

      if (warm_up) {
        std::cerr << "Warm-up reads..." << std::endl;
        err_count = 0;
        for (size_t i = 0; i < warm_up_ops && time_bound(start_us, max_us); i++) {
          try {
            s_if->read(key_gen->next());
          } catch (std::runtime_error &e) {
            --i;
            ++err_count;
            if (err_count > ERROR_MAX) {
              std::cerr << "Too many errors" << std::endl;
              s_if->destroy();
              std::cerr << "Destroyed storage interface." << std::endl;
              exit(1);
            }
          }
        }
      }

      std::cerr << "Starting reads..." << std::endl;
      auto r_begin = now_us();
      for (size_t i = 0; i < num_ops && time_bound(start_us, max_us); ++i) {
        auto t_b = now_us();
        try {
          s_if->read(key_gen->next());
        } catch (std::runtime_error &e) {
          --i;
          ++err_count;
          if (err_count > ERROR_MAX) {
            std::cerr << "Too many errors" << std::endl;
            s_if->destroy();
            std::cerr << "Destroyed storage interface." << std::endl;
            exit(1);
          }
        }
        auto t_e = now_us();
        lr << (t_e - t_b) << std::endl;
      }
      auto r_end = now_us();
      auto r_elapsed_s = static_cast<double>(r_end - r_begin) / 1000000.0;
      std::cerr << "Finished reads." << std::endl;

      tr << (static_cast<double>(num_ops) / r_elapsed_s) << std::endl;
      lr.close();
      tr.close();
    }

    if ((mode & BENCHMARK_DESTROY) == BENCHMARK_DESTROY) {
      s_if->destroy();
      std::cerr << "Destroyed storage interface." << std::endl;
    }
  }

  static bool time_bound(uint64_t start_us, uint64_t max_us) {
    if (now_us() - start_us < max_us)
      return true;
    std::cerr << "WARN Benchmark timed out..." << std::endl;
    return false;
  }

  static uint64_t now_us() {
    using namespace std::chrono;
    time_point<system_clock> now = system_clock::now();
    return static_cast<uint64_t>(duration_cast<microseconds>(now.time_since_epoch()).count());
  }
};

#endif //STORAGE_BENCH_BENCHMARK_H
