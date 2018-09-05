#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <aws/core/Aws.h>
#include "storage_interface.h"
#include "benchmark.h"
#include "key_generator.h"

#define LAMBDA_TIMEOUT_SAFE 240

int main(int argc, char **argv) {
  if (argc != 10) {
    std::cerr << "Usage: " << argv[0] << " id system conf_file output_prefix value_size mode num_ops warm_up dist"
              << std::endl;
    return -1;
  }

  Aws::SDKOptions m_options;
  m_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
  Aws::InitAPI(m_options);

  std::string id = argv[1];
  std::string system = argv[2];
  std::string conf_file = argv[3];
  std::string result_prefix = argv[4];
  size_t value_size = std::stoull(argv[5]);
  int32_t mode = 0;
  bool async = false;
  size_t rate = 0;
  std::string m(argv[6]);
  if (m.find("read") != std::string::npos) {
    mode |= BENCHMARK_READ;
  }
  if (m.find("write") != std::string::npos) {
    mode |= BENCHMARK_WRITE;
  }
  if (m.find("create") != std::string::npos) {
    mode |= BENCHMARK_CREATE;
  }
  if (m.find("destroy") != std::string::npos) {
    mode |= BENCHMARK_DESTROY;
  }
  size_t async_pos;
  if ((async_pos = m.find("async{")) != std::string::npos) {
    size_t rbeg = async_pos + 6;
    size_t rend = m.find('}', rbeg);
    size_t len = rend - rbeg;
    rate = static_cast<size_t>(std::stoll(m.substr(rbeg, len)));
    std::cerr << "Rate: " << rate << std::endl;
    async = true;
  }
  size_t n_ops = std::stoull(argv[7]);
  bool warm_up = static_cast<bool>(std::stod(argv[8]));

  namespace pt = boost::property_tree;
  pt::ptree conf;
  try {
    pt::ini_parser::read_ini(conf_file, conf);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  auto s_if = storage_interfaces::get_interface(system);
  auto s_conf = conf.get_child(system);
  auto b_conf = conf.get_child("benchmark");
  std::string output_prefix = result_prefix + "_" + std::to_string(value_size);
  uint64_t timeout = b_conf.get<uint64_t>("timeout", LAMBDA_TIMEOUT_SAFE) * 1000 * 1000;
  std::string control_host = b_conf.get<std::string>("control_host", "localhost");
  int control_port = b_conf.get<int>("control_port", 8889);
  if (!strcmp(argv[9], "zipf")) {
    auto begin = benchmark::now_us();
    auto key_gen = std::make_shared<zipf_key_generator>(0.0, n_ops);
    auto remaining = timeout - (benchmark::now_us() - begin);
    if (async) {
      benchmark::run_async(s_if,
                           s_conf,
                           key_gen,
                           output_prefix,
                           value_size,
                           n_ops,
                           rate,
                           warm_up,
                           mode,
                           remaining,
                           control_host,
                           control_port,
                           id);
    } else {
      benchmark::run(s_if,
                     s_conf,
                     key_gen,
                     output_prefix,
                     value_size,
                     n_ops,
                     warm_up,
                     mode,
                     remaining,
                     control_host,
                     control_port,
                     id);
    }
  } else if (!strcmp(argv[9], "sequential")) {
    auto begin = benchmark::now_us();
    auto key_gen = std::make_shared<sequential_key_generator>();
    auto remaining = timeout - (benchmark::now_us() - begin);
    if (async) {
      benchmark::run_async(s_if,
                           s_conf,
                           key_gen,
                           output_prefix,
                           value_size,
                           n_ops,
                           rate,
                           warm_up,
                           mode,
                           remaining,
                           control_host,
                           control_port,
                           id);
    } else {
      benchmark::run(s_if,
                     s_conf,
                     key_gen,
                     output_prefix,
                     value_size,
                     n_ops,
                     warm_up,
                     mode,
                     remaining,
                     control_host,
                     control_port,
                     id);
    }
  } else {
    std::cerr << "Unknown key distribution: " << argv[8] << std::endl;
  }

  Aws::ShutdownAPI(m_options);

  return 0;
}