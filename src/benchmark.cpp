
#include <stdlib.h>
#include <fstream>
#include <chrono>
#include "benchmark.h"

void benchmark::run(const std::shared_ptr<storage_interface> &iface,
                    const std::string &output_path,
                    size_t value_size,
                    size_t num_ops,
                    const storage_interface::property_map &conf) {
  int err_count = 0;
  std::ofstream lr(output_path + "_read_latency.txt");
  std::ofstream lw(output_path + "_write_latency.txt");
  std::ofstream tr(output_path + "_read_throughput.txt");
  std::ofstream tw(output_path + "_write_throughput.txt");
  std::string value(value_size, 'x');

  std::cerr << "Initializing storage interface..." << std::endl;
  iface->init(conf);

  // Warmup
  std::cerr << "Warmup writes..." << std::endl;
  size_t warmup_ops = num_ops / 10;
  for (size_t i = 0; i < warmup_ops; i++) {
    try {
      iface->write(std::to_string(i + num_ops), value);
    } catch (std::runtime_error &e) {
      --i;
      ++err_count;
      if (err_count > ERROR_MAX) {
        std::cerr << "Too many errors" << std::endl;
        iface->destroy();
        std::cerr << "Destroyed storage interface." << std::endl;
        exit(-1);
      }
    }
  }

  std::cerr << "Starting writes..." << std::endl;
  auto w_begin = now_us();
  for (size_t i = 0; i < num_ops; ++i) {
    auto t_b = now_us();
    try {
      iface->write(std::to_string(i), value);
    } catch (std::runtime_error &e) {
      --i;
      ++err_count;
      if (err_count > ERROR_MAX) {
        std::cerr << "Too many errors" << std::endl;
        iface->destroy();
        std::cerr << "Destroyed storage interface." << std::endl;
        exit(-1);
      }
    }
    auto t_e = now_us();
    lw << (t_e - t_b) << std::endl;
  }
  auto w_end = now_us();
  std::cerr << "Finished writes, warmup reads..." << std::endl;
  err_count = 0;
  for (size_t i = 0; i < warmup_ops; i++) {
    try {
      iface->read(std::to_string(i + num_ops));
    } catch (std::runtime_error &e) {
      --i;
      ++err_count;
      if (err_count > ERROR_MAX) {
        std::cerr << "Too many errors" << std::endl;
        iface->destroy();
        std::cerr << "Destroyed storage interface." << std::endl;
        exit(-1);
      }
    }
  }
  std::cerr << "Starting reads..." << std::endl;
  auto r_begin = now_us();
  for (size_t i = 0; i < num_ops; ++i) {
    auto t_b = now_us();
    try {
      iface->read(std::to_string(i));
    } catch (std::runtime_error &e) {
      --i;
      ++err_count;
      if (err_count > ERROR_MAX) {
        std::cerr << "Too many errors" << std::endl;
        iface->destroy();
        std::cerr << "Destroyed storage interface." << std::endl;
        exit(-1);
      }
    }
    auto t_e = now_us();
    lr << (t_e - t_b) << std::endl;
  }
  auto r_end = now_us();

  std::cerr << "Finished benchmark." << std::endl;
  auto w_elapsed_s = static_cast<double>(w_end - w_begin) / 1000000.0;
  auto r_elapsed_s = static_cast<double>(r_end - r_begin) / 1000000.0;
  tw << (static_cast<double>(num_ops) / w_elapsed_s) << std::endl;
  tr << (static_cast<double>(num_ops) / r_elapsed_s) << std::endl;

  lr.close();
  lw.close();
  tr.close();
  tw.close();

  iface->destroy();
  std::cerr << "Destroyed storage interface." << std::endl;
}

uint64_t benchmark::now_us() {
  std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
}
