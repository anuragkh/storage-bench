#ifndef STORAGE_BENCH_BENCHMARK_H
#define STORAGE_BENCH_BENCHMARK_H

#include <cstdlib>
#include <chrono>
#include <fstream>
#include <memory>
#include <thread>
#include "storage_interface.h"
#include "rate_limiter.h"
#include "benchmark_utils.h"
#include "key_generator.h"
#include "notification_interface.h"

#ifndef ERROR_MAX
#define ERROR_MAX 1000
#endif

#ifndef MEASURE_INTERVAL
#define MEASURE_INTERVAL 1000000
#endif

#define BENCHMARK_READ    1
#define BENCHMARK_WRITE   2
#define BENCHMARK_CREATE  4
#define BENCHMARK_DESTROY 8

class benchmark {
 public:
  template<typename K>
  static void run(const std::shared_ptr<storage_interface> &s_if,
                  const storage_interface::property_map &conf,
                  const std::shared_ptr<K> key_gen,
                  const std::string &output_path,
                  size_t value_size,
                  size_t num_ops,
                  bool warm_up,
                  int32_t mode,
                  uint64_t max_us,
                  const std::string &control_host,
                  int control_port,
                  const std::string &id) {
    int err_count = 0;
    size_t warm_up_ops = num_ops / 10;
    std::string value(value_size, 'x');

    auto start_us = benchmark_utils::now_us();

    std::cerr << "Initializing storage interface..." << std::endl;
    s_if->init(conf, (mode & BENCHMARK_CREATE) == BENCHMARK_CREATE);

    benchmark_utils::signal(control_host, control_port, id);

    if ((mode & BENCHMARK_WRITE) == BENCHMARK_WRITE) {
      std::ofstream lw(output_path + "_write_latency.txt");
      std::ofstream tw(output_path + "_write_throughput.txt");

      if (warm_up) {
        std::cerr << "Warm-up writes..." << std::endl;
        for (size_t i = 0; i < warm_up_ops && benchmark_utils::time_bound(start_us, max_us); i++) {
          try {
            s_if->write(key_gen->next(), value);
          } catch (std::runtime_error &e) {
            --i;
            ++err_count;
            if (err_count > ERROR_MAX) {
              std::cerr << "Too many errors" << std::endl;
              std::cerr << "Last error: " << e.what() << std::endl;
              s_if->destroy();
              std::cerr << "Destroyed storage interface." << std::endl;
              exit(1);
            }
          }
        }
      }

      std::cerr << "Starting writes..." << std::endl;
      auto w_begin = benchmark_utils::now_us();
      size_t i;
      for (i = 0; i < num_ops && benchmark_utils::time_bound(start_us, max_us); ++i) {
        auto t_b = benchmark_utils::now_us();
        try {
          s_if->write(key_gen->next(), value);
        } catch (std::runtime_error &e) {
          --i;
          ++err_count;
          if (err_count > ERROR_MAX) {
            std::cerr << "Too many errors" << std::endl;
            std::cerr << "Last error: " << e.what() << std::endl;
            s_if->destroy();
            std::cerr << "Destroyed storage interface." << std::endl;
            exit(1);
          }
        }
        auto t_e = benchmark_utils::now_us();
        lw << t_e << "\t" << (t_e - t_b) << std::endl;
      }
      auto w_end = benchmark_utils::now_us();
      auto w_elapsed_s = static_cast<double>(w_end - w_begin) / 1000000.0;
      std::cerr << "Finished writes." << std::endl;

      tw << (static_cast<double>(i) / w_elapsed_s) << std::endl;
      lw.close();
      tw.close();
    }

    key_gen->reset();

    if ((mode & BENCHMARK_READ) == BENCHMARK_READ) {
      std::ofstream lr(output_path + "_read_latency.txt");
      std::ofstream tr(output_path + "_read_throughput.txt");

      if (warm_up) {
        std::cerr << "Warm-up reads..." << std::endl;
        err_count = 0;
        for (size_t i = 0; i < warm_up_ops && benchmark_utils::time_bound(start_us, max_us); i++) {
          try {
            s_if->read(key_gen->next());
          } catch (std::runtime_error &e) {
            --i;
            ++err_count;
            if (err_count > ERROR_MAX) {
              std::cerr << "Too many errors" << std::endl;
              std::cerr << "Last error: " << e.what() << std::endl;
              s_if->destroy();
              std::cerr << "Destroyed storage interface." << std::endl;
              exit(1);
            }
          }
        }
      }

      std::cerr << "Starting reads..." << std::endl;
      auto r_begin = benchmark_utils::now_us();
      size_t i;
      for (i = 0; i < num_ops && benchmark_utils::time_bound(start_us, max_us); ++i) {
        auto t_b = benchmark_utils::now_us();
        try {
          s_if->read(key_gen->next());
        } catch (std::runtime_error &e) {
          --i;
          ++err_count;
          if (err_count > ERROR_MAX) {
            std::cerr << "Too many errors" << std::endl;
            std::cerr << "Last error: " << e.what() << std::endl;
            s_if->destroy();
            std::cerr << "Destroyed storage interface." << std::endl;
            exit(1);
          }
        }
        auto t_e = benchmark_utils::now_us();
        lr << t_e << "\t" << (t_e - t_b) << std::endl;
      }
      auto r_end = benchmark_utils::now_us();
      auto r_elapsed_s = static_cast<double>(r_end - r_begin) / 1000000.0;
      std::cerr << "Finished reads." << std::endl;

      tr << (static_cast<double>(i) / r_elapsed_s) << std::endl;
      lr.close();
      tr.close();
    }

    if ((mode & BENCHMARK_DESTROY) == BENCHMARK_DESTROY) {
      s_if->destroy();
      std::cerr << "Destroyed storage interface." << std::endl;
    }
  }

  static void benchmark_notifications(const std::shared_ptr<notification_interface> &s_if,
                                      const storage_interface::property_map &conf,
                                      const std::string &output_path,
                                      size_t value_size,
                                      size_t num_ops,
                                      int32_t mode,
                                      uint64_t max_us,
                                      const std::string &control_host,
                                      int control_port,
                                      const std::string &id) {
    sequential_key_generator msg_gen;
    int err_count = 0;

    auto start_us = benchmark_utils::now_us();

    std::cerr << "Initializing storage interface..." << std::endl;
    s_if->init(conf, (mode & BENCHMARK_CREATE) == BENCHMARK_CREATE);

    auto num_listeners = conf.get<size_t>("num_listeners");
    auto channel = conf.get<std::string>("channel");
    for (size_t i = 0; i < num_listeners; i++) {
      s_if->subscribe(channel);
    }

    benchmark_utils::signal(control_host, control_port, id);

    std::cerr << "Publishing messages..." << std::endl;
    for (size_t i = 0; i < num_ops && benchmark_utils::time_bound(start_us, max_us); ++i) {
      try {
        s_if->publish(channel, msg_gen.next(static_cast<int>(value_size)));
      } catch (std::runtime_error &e) {
        --i;
        ++err_count;
        if (err_count > ERROR_MAX) {
          std::cerr << "Too many errors" << std::endl;
          std::cerr << "Last error: " << e.what() << std::endl;
          s_if->destroy();
          std::cerr << "Destroyed storage interface." << std::endl;
          exit(1);
        }
      }
    }

    for (size_t i = 0; i < num_listeners; ++i) {
      s_if->wait(i);
    }

    auto res = s_if->get_latencies();
    for (size_t i = 0; i < num_listeners; ++i) {
      auto r = res[i];
      std::ofstream out(output_path + "_" + std::to_string(i) + ".txt");
      for (const auto &l: r) {
        out << l << "\n";
      }
    }

    if ((mode & BENCHMARK_DESTROY) == BENCHMARK_DESTROY) {
      s_if->destroy();
      std::cerr << "Destroyed storage interface." << std::endl;
    }
  }

  template<typename K>
  static void async_writes(const std::shared_ptr<storage_interface> &s_if,
                           const std::shared_ptr<K> key_gen,
                           const std::string &output_path,
                           size_t value_size,
                           size_t num_ops,
                           size_t n_async,
                           bool warm_up,
                           uint64_t start_us,
                           uint64_t max_us) {
    int err_count = 0;
    size_t warm_up_ops = num_ops / 10;
    std::string value(value_size, 'x');
    std::ofstream tw(output_path + "_write.txt");
    if (warm_up) {
      std::cerr << "Warm-up writes..." << std::endl;
      for (size_t i = 0; i < warm_up_ops && benchmark_utils::time_bound(start_us, max_us); i += n_async) {
        try {
          for (size_t j = 0; j < n_async; ++j)
            s_if->write_async(key_gen->next(), value);
          for (size_t j = 0; j < n_async; ++j)
            s_if->wait_write();
        } catch (std::runtime_error &e) {
          --i;
          ++err_count;
          if (err_count > ERROR_MAX) {
            std::cerr << "Too many errors" << std::endl;
            std::cerr << "Last error: " << e.what() << std::endl;
            s_if->destroy();
            std::cerr << "Destroyed storage interface." << std::endl;
            exit(1);
          }
        }
      }
    }

    std::cerr << "Starting writes..." << std::endl;
    auto w_begin = benchmark_utils::now_us();
    size_t i;
    auto last_measure_time = w_begin;
    size_t writes = 0;
    tw << w_begin << "\t" << writes << std::endl;
    for (i = 0; i < num_ops && benchmark_utils::time_bound(start_us, max_us); i += n_async) {
      try {
        for (size_t j = 0; j < n_async; ++j)
          s_if->write_async(key_gen->next(), value);
        for (size_t j = 0; j < n_async; ++j)
          s_if->wait_write();
        writes += n_async;
      } catch (std::runtime_error &e) {
        --i;
        ++err_count;
        if (err_count > ERROR_MAX) {
          std::cerr << "Too many errors" << std::endl;
          std::cerr << "Last error: " << e.what() << std::endl;
          s_if->destroy();
          std::cerr << "Destroyed storage interface." << std::endl;
          exit(1);
        }
      }
      uint64_t cur_time;
      if ((cur_time = benchmark_utils::now_us()) - last_measure_time >= MEASURE_INTERVAL) {
        tw << cur_time << "\t" << writes << std::endl;
        writes = 0;
        last_measure_time = cur_time;
      }
    }
    uint64_t w_end = benchmark_utils::now_us();
    tw << w_end << "\t" << writes << std::endl;
    tw.close();
    std::cerr << "Finished writes." << std::endl;
  }

  template<typename K>
  static void async_reads(const std::shared_ptr<storage_interface> &s_if,
                          const std::shared_ptr<K> key_gen,
                          const std::string &output_path,
                          size_t num_ops,
                          size_t n_async,
                          bool warm_up,
                          uint64_t start_us,
                          uint64_t max_us) {
    int err_count = 0;
    size_t warm_up_ops = num_ops / 10;
    std::ofstream tr(output_path + "_read.txt");
    if (warm_up) {
      std::cerr << "Warm-up reads..." << std::endl;
      for (size_t i = 0; i < warm_up_ops && benchmark_utils::time_bound(start_us, max_us); i += n_async) {
        try {
          for (size_t j = 0; j < n_async; ++j)
            s_if->read_async(key_gen->next());
          for (size_t j = 0; j < n_async; ++j)
            s_if->wait_read();
        } catch (std::runtime_error &e) {
          --i;
          ++err_count;
          if (err_count > ERROR_MAX) {
            std::cerr << "Too many errors" << std::endl;
            std::cerr << "Last error: " << e.what() << std::endl;
            s_if->destroy();
            std::cerr << "Destroyed storage interface." << std::endl;
            exit(1);
          }
        }
      }
    }

    std::cerr << "Starting reads..." << std::endl;
    auto r_begin = benchmark_utils::now_us();
    size_t i;
    auto last_measure_time = r_begin;
    size_t reads = 0;
    tr << r_begin << "\t" << reads << std::endl;
    for (i = 0; i < num_ops && benchmark_utils::time_bound(start_us, max_us); i += n_async) {
      try {
        for (size_t j = 0; j < n_async; ++j)
          s_if->read_async(key_gen->next());
        for (size_t j = 0; j < n_async; ++j)
          s_if->wait_read();
        reads += n_async;
      } catch (std::runtime_error &e) {
        --i;
        ++err_count;
        if (err_count > ERROR_MAX) {
          std::cerr << "Too many errors" << std::endl;
          std::cerr << "Last error: " << e.what() << std::endl;
          s_if->destroy();
          std::cerr << "Destroyed storage interface." << std::endl;
          exit(1);
        }
      }
      uint64_t cur_time;
      if ((cur_time = benchmark_utils::now_us()) - last_measure_time >= MEASURE_INTERVAL) {
        tr << cur_time << "\t" << reads << std::endl;
        last_measure_time = cur_time;
      }
    }
    uint64_t r_end = benchmark_utils::now_us();
    tr << r_end << "\t" << reads << std::endl;
    tr.close();
    std::cerr << "Finished reads." << std::endl;
  }

  template<typename K>
  static void send_writes(const std::shared_ptr<storage_interface> &s_if,
                          const std::shared_ptr<K> key_gen,
                          const std::shared_ptr<rate_limiter> &limiter,
                          const std::string &output_path,
                          size_t value_size,
                          size_t num_ops,
                          bool warm_up,
                          uint64_t start_us,
                          uint64_t max_us) {
    int err_count = 0;
    size_t warm_up_ops = num_ops / 10;
    std::string value(value_size, 'x');
    std::ofstream tw(output_path + "_write_send.txt");
    if (warm_up) {
      std::cerr << "[SEND] Warm-up writes..." << std::endl;
      for (size_t i = 0; i < warm_up_ops && benchmark_utils::time_bound(start_us, max_us); i++) {
        try {
          limiter->acquire();
          s_if->write_async(key_gen->next(), value);
        } catch (std::runtime_error &e) {
          --i;
          ++err_count;
          if (err_count > ERROR_MAX) {
            std::cerr << "[SEND] Too many errors" << std::endl;
            std::cerr << "Last error: " << e.what() << std::endl;
            s_if->destroy();
            std::cerr << "[SEND] Destroyed storage interface." << std::endl;
            exit(1);
          }
        }
      }
    }

    std::cerr << "[SEND] Starting writes..." << std::endl;
    auto w_begin = benchmark_utils::now_us();
    size_t i;
    auto last_measure_time = w_begin;
    size_t interval_sent = 0;
    for (i = 0; i < num_ops && benchmark_utils::time_bound(start_us, max_us); ++i) {
      try {
        limiter->acquire();
        s_if->write_async(key_gen->next(), value);
        ++interval_sent;
      } catch (std::runtime_error &e) {
        --i;
        ++err_count;
        if (err_count > ERROR_MAX) {
          std::cerr << "[SEND] Too many errors" << std::endl;
          std::cerr << "Last error: " << e.what() << std::endl;
          s_if->destroy();
          std::cerr << "[SEND] Destroyed storage interface." << std::endl;
          exit(1);
        }
      }
      uint64_t cur_time;
      if ((cur_time = benchmark_utils::now_us()) - last_measure_time >= MEASURE_INTERVAL) {
        double diff = cur_time - last_measure_time;
        double send_rate = ((double) interval_sent * 1000.0 * 1000.0) / diff;
        tw << cur_time << "\t" << send_rate << std::endl;
        interval_sent = 0;
        last_measure_time = cur_time;
      }
    }
    uint64_t cur_time = benchmark_utils::now_us();
    double diff = cur_time - last_measure_time;
    double send_rate = ((double) interval_sent * 1000.0 * 1000.0) / diff;
    tw << cur_time << "\t" << send_rate << std::endl;
    tw.close();
    std::cerr << "[SEND] Finished writes." << std::endl;
  }

  static void recv_writes(const std::shared_ptr<storage_interface> &s_if,
                          const std::string &output_path,
                          size_t num_ops,
                          bool warm_up,
                          uint64_t start_us,
                          uint64_t max_us) {
    int err_count = 0;
    size_t warm_up_ops = num_ops / 10;
    std::ofstream tw(output_path + "_write_recv.txt");
    if (warm_up) {
      std::cerr << "[RECV] Warm-up writes..." << std::endl;
      for (size_t i = 0; i < warm_up_ops && benchmark_utils::time_bound(start_us, max_us); i++) {
        try {
          s_if->wait_write();
        } catch (std::runtime_error &e) {
          --i;
          ++err_count;
          if (err_count > ERROR_MAX) {
            std::cerr << "[RECV] Too many errors" << std::endl;
            std::cerr << "Last error: " << e.what() << std::endl;
            s_if->destroy();
            std::cerr << "[RECV] Destroyed storage interface." << std::endl;
            exit(1);
          }
        }
      }
    }

    std::cerr << "[RECV] Starting writes..." << std::endl;
    auto w_begin = benchmark_utils::now_us();
    size_t i;
    auto last_measure_time = w_begin;
    size_t interval_recv = 0;
    for (i = 0; i < num_ops && benchmark_utils::time_bound(start_us, max_us); ++i) {
      try {
        s_if->wait_write();
        ++interval_recv;
      } catch (std::runtime_error &e) {
        --i;
        ++err_count;
        if (err_count > ERROR_MAX) {
          std::cerr << "[RECV] Too many errors" << std::endl;
          std::cerr << "Last error: " << e.what() << std::endl;
          s_if->destroy();
          std::cerr << "[RECV] Destroyed storage interface." << std::endl;
          exit(1);
        }
      }
      uint64_t cur_time;
      if ((cur_time = benchmark_utils::now_us()) - last_measure_time >= MEASURE_INTERVAL) {
        double diff = cur_time - last_measure_time;
        double send_rate = ((double) interval_recv * 1000.0 * 1000.0) / diff;
        tw << cur_time << "\t" << send_rate << std::endl;
        interval_recv = 0;
        last_measure_time = cur_time;
      }
    }
    uint64_t cur_time = benchmark_utils::now_us();
    double diff = cur_time - last_measure_time;
    double send_rate = ((double) interval_recv * 1000.0 * 1000.0) / diff;
    tw << cur_time << "\t" << send_rate << std::endl;
    tw.close();

    std::cerr << "[RECV] Finished writes." << std::endl;
  }

  template<typename K>
  static void send_reads(const std::shared_ptr<storage_interface> &s_if,
                         const std::shared_ptr<K> key_gen,
                         const std::shared_ptr<rate_limiter> &limiter,
                         const std::string &output_path,
                         size_t num_ops,
                         bool warm_up,
                         uint64_t start_us,
                         uint64_t max_us) {
    int err_count = 0;
    size_t warm_up_ops = num_ops / 10;
    std::ofstream tr(output_path + "_read_send.txt");
    if (warm_up) {
      std::cerr << "[SEND] Warm-up reads..." << std::endl;
      for (size_t i = 0; i < warm_up_ops && benchmark_utils::time_bound(start_us, max_us); i++) {
        try {
          limiter->acquire();
          s_if->read_async(key_gen->next());
        } catch (std::runtime_error &e) {
          --i;
          ++err_count;
          if (err_count > ERROR_MAX) {
            std::cerr << "[SEND] Too many errors" << std::endl;
            std::cerr << "Last error: " << e.what() << std::endl;
            s_if->destroy();
            std::cerr << "[SEND] Destroyed storage interface." << std::endl;
            exit(1);
          }
        }
      }
    }

    std::cerr << "[SEND] Starting reads..." << std::endl;
    auto r_begin = benchmark_utils::now_us();
    size_t i;
    auto last_measure_time = r_begin;
    size_t interval_sent = 0;
    for (i = 0; i < num_ops && benchmark_utils::time_bound(start_us, max_us); ++i) {
      try {
        limiter->acquire();
        s_if->read_async(key_gen->next());
        ++interval_sent;
      } catch (std::runtime_error &e) {
        --i;
        ++err_count;
        if (err_count > ERROR_MAX) {
          std::cerr << "[SEND] Too many errors" << std::endl;
          std::cerr << "Last error: " << e.what() << std::endl;
          s_if->destroy();
          std::cerr << "[SEND] Destroyed storage interface." << std::endl;
          exit(1);
        }
      }
      uint64_t cur_time;
      if ((cur_time = benchmark_utils::now_us()) - last_measure_time >= MEASURE_INTERVAL) {
        double diff = cur_time - last_measure_time;
        double send_rate = ((double) interval_sent * 1000.0 * 1000.0) / diff;
        tr << cur_time << "\t" << send_rate << std::endl;
        interval_sent = 0;
        last_measure_time = cur_time;
      }
    }
    uint64_t cur_time = benchmark_utils::now_us();
    double diff = cur_time - last_measure_time;
    double send_rate = ((double) interval_sent * 1000.0 * 1000.0) / diff;
    tr << cur_time << "\t" << send_rate << std::endl;
    tr.close();
    std::cerr << "[SEND] Finished reads." << std::endl;
  }

  static void recv_reads(const std::shared_ptr<storage_interface> &s_if,
                         const std::string &output_path,
                         size_t num_ops,
                         bool warm_up,
                         uint64_t start_us,
                         uint64_t max_us) {
    int err_count = 0;
    size_t warm_up_ops = num_ops / 10;
    std::ofstream tr(output_path + "_read_recv.txt");
    if (warm_up) {
      std::cerr << "[RECV] Warm-up reads..." << std::endl;
      for (size_t i = 0; i < warm_up_ops && benchmark_utils::time_bound(start_us, max_us); i++) {
        try {
          s_if->wait_read();
        } catch (std::runtime_error &e) {
          --i;
          ++err_count;
          if (err_count > ERROR_MAX) {
            std::cerr << "[RECV] Too many errors" << std::endl;
            std::cerr << "Last error: " << e.what() << std::endl;
            s_if->destroy();
            std::cerr << "[RECV] Destroyed storage interface." << std::endl;
            exit(1);
          }
        }
      }
    }

    std::cerr << "[RECV] Starting reads..." << std::endl;
    auto r_begin = benchmark_utils::now_us();
    size_t i;
    auto last_measure_time = r_begin;
    size_t interval_recv = 0;
    for (i = 0; i < num_ops && benchmark_utils::time_bound(start_us, max_us); ++i) {
      try {
        s_if->wait_read();
        ++interval_recv;
      } catch (std::runtime_error &e) {
        --i;
        ++err_count;
        if (err_count > ERROR_MAX) {
          std::cerr << "[RECV] Too many errors" << std::endl;
          std::cerr << "Last error: " << e.what() << std::endl;
          s_if->destroy();
          std::cerr << "[RECV] Destroyed storage interface." << std::endl;
          exit(1);
        }
      }
      uint64_t cur_time;
      if ((cur_time = benchmark_utils::now_us()) - last_measure_time >= MEASURE_INTERVAL) {
        double diff = cur_time - last_measure_time;
        double recv_rate = ((double) interval_recv * 1000.0 * 1000.0) / diff;
        tr << cur_time << "\t" << recv_rate << std::endl;
        interval_recv = 0;
        last_measure_time = cur_time;
      }
    }
    uint64_t cur_time = benchmark_utils::now_us();
    double diff = cur_time - last_measure_time;
    double send_rate = ((double) interval_recv * 1000.0 * 1000.0) / diff;
    tr << cur_time << "\t" << send_rate << std::endl;
    tr.close();
    std::cerr << "[RECV] Finished reads." << std::endl;
  }

  template<typename K>
  static void run_rate_limited(const std::shared_ptr<storage_interface> &s_if,
                               const storage_interface::property_map &conf,
                               const std::shared_ptr<K> key_gen,
                               const std::string &output_path,
                               double rate,
                               size_t value_size,
                               size_t num_ops,
                               bool warm_up,
                               int32_t mode,
                               uint64_t max_us,
                               const std::string &control_host,
                               int control_port,
                               const std::string &id) {

    auto start_us = benchmark_utils::now_us();

    std::cerr << "Initializing storage interface..." << std::endl;
    s_if->init(conf, (mode & BENCHMARK_CREATE) == BENCHMARK_CREATE);

    benchmark_utils::signal(control_host, control_port, id);

    if ((mode & BENCHMARK_WRITE) == BENCHMARK_WRITE) {
      std::thread recv_thread([=]() {
        benchmark::recv_writes(s_if, output_path, num_ops, warm_up, start_us, max_us);
      });
      benchmark::send_writes(s_if,
                             key_gen,
                             std::make_shared<rate_limiter>(rate),
                             output_path,
                             value_size,
                             num_ops,
                             warm_up,
                             start_us,
                             max_us);
      recv_thread.join();
    }

    key_gen->reset();

    if ((mode & BENCHMARK_READ) == BENCHMARK_READ) {
      std::thread recv_thread([=] {
        benchmark::recv_reads(s_if, output_path, num_ops, warm_up, start_us, max_us);
      });
      benchmark::send_reads(s_if,
                            key_gen,
                            std::make_shared<rate_limiter>(rate),
                            output_path,
                            num_ops,
                            warm_up,
                            start_us,
                            max_us);
      recv_thread.join();
    }

    if ((mode & BENCHMARK_DESTROY) == BENCHMARK_DESTROY) {
      s_if->destroy();
    }
  }

  template<typename K>
  static void run_async(const std::shared_ptr<storage_interface> &s_if,
                        const storage_interface::property_map &conf,
                        const std::shared_ptr<K> key_gen,
                        const std::string &output_path,
                        size_t value_size,
                        size_t num_ops,
                        size_t n_async,
                        bool warm_up,
                        int32_t mode,
                        uint64_t max_us,
                        const std::string &control_host,
                        int control_port,
                        const std::string &id) {
    auto start_us = benchmark_utils::now_us();

    std::cerr << "Initializing storage interface..." << std::endl;
    s_if->init(conf, (mode & BENCHMARK_CREATE) == BENCHMARK_CREATE);

    benchmark_utils::signal(control_host, control_port, id);
    if ((mode & BENCHMARK_WRITE) == BENCHMARK_WRITE)
      benchmark::async_writes(s_if, key_gen, output_path, value_size, num_ops, n_async, warm_up, start_us, max_us);

    key_gen->reset();

    if ((mode & BENCHMARK_READ) == BENCHMARK_READ)
      benchmark::async_reads(s_if, key_gen, output_path, num_ops, n_async, warm_up, start_us, max_us);

    if ((mode & BENCHMARK_DESTROY) == BENCHMARK_DESTROY)
      s_if->destroy();
  }
};

#endif //STORAGE_BENCH_BENCHMARK_H
