
#include <stdlib.h>
#include <fstream>
#include <chrono>
#include "benchmark.h"

void benchmark::run(const std::shared_ptr<storage_interface> &s_if,
                    const storage_interface::property_map &conf,
                    const std::string &output_path,
                    size_t value_size,
                    size_t num_ops,
                    bool warmup,
                    int32_t mode) {
  int err_count = 0;
  size_t warmup_ops = num_ops / 10;
  std::string value(value_size, 'x');

  std::cerr << "Initializing storage interface..." << std::endl;
  s_if->init(conf);

  if ((mode & BENCHMARK_WRITE) == BENCHMARK_WRITE) {
    std::ofstream lw(output_path + "_write_latency.txt");
    std::ofstream tw(output_path + "_write_throughput.txt");

    if (warmup) {
      std::cerr << "Warmup writes..." << std::endl;
      for (size_t i = 0; i < warmup_ops; i++) {
        try {
          s_if->write(std::to_string(i + num_ops), value);
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
    for (size_t i = 0; i < num_ops; ++i) {
      auto t_b = now_us();
      try {
        s_if->write(std::to_string(i), value);
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

    if (warmup) {
      std::cerr << "Warmup reads..." << std::endl;
      err_count = 0;
      for (size_t i = 0; i < warmup_ops; i++) {
        try {
          s_if->read(std::to_string(i + num_ops));
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
    for (size_t i = 0; i < num_ops; ++i) {
      auto t_b = now_us();
      try {
        s_if->read(std::to_string(i));
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

  s_if->destroy();
  std::cerr << "Destroyed storage interface." << std::endl;
}

uint64_t benchmark::now_us() {
  using namespace std::chrono;
  time_point<system_clock> now = system_clock::now();
  return static_cast<uint64_t>(duration_cast<microseconds>(now.time_since_epoch()).count());
}
