#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <aws/core/Aws.h>
#include "storage_interface.h"
#include "benchmark.h"

#define LAMBDA_TIMEOUT_SAFE 240

int main(int argc, char **argv) {
  if (argc != 10) {
    std::cerr << "Usage: " << argv[0] << " id system conf_file output_prefix value_size mode num_ops"
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
  size_t n_ops = std::stoull(argv[7]);

  namespace pt = boost::property_tree;
  pt::ptree conf;
  try {
    pt::ini_parser::read_ini(conf_file, conf);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  char hbuf[1024];
  gethostname(hbuf, sizeof(hbuf));

  auto s_if = notification_interfaces::get_interface(system);
  auto s_conf = conf.get_child(system);
  auto b_conf = conf.get_child("benchmark");
  std::string output_prefix = result_prefix + "_" + std::to_string(value_size);
  uint64_t timeout = b_conf.get<uint64_t>("timeout", LAMBDA_TIMEOUT_SAFE) * 1000 * 1000;
  std::string control_host = b_conf.get<std::string>("control_host", hbuf);
  int control_port = b_conf.get<int>("control_port", 8889);
  auto begin = benchmark_utils::now_us();
  auto key_gen = std::make_shared<zipf_key_generator>(0.0, n_ops);
  auto remaining = timeout - (benchmark_utils::now_us() - begin);

  benchmark::benchmark_notifications(s_if,
                 s_conf,
                 output_prefix,
                 value_size,
                 n_ops,
                 mode,
                 remaining,
                 control_host,
                 control_port,
                 id);

  Aws::ShutdownAPI(m_options);

  return 0;
}