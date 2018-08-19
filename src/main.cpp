#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "storage_interface.h"
#include "benchmark.h"
#include "key_generator.h"

#define LAMBDA_TIMEOUT_SAFE (240 * 1000 * 1000)

int main(int argc, char **argv) {
  if (argc != 9) {
    std::cerr << "Usage: " << argv[0] << " system conf_file output_prefix value_size mode num_ops warm_up dist"
              << std::endl;
    return -1;
  }
  std::string system = argv[1];
  std::string conf_file = argv[2];
  std::string result_prefix = argv[3];
  size_t value_size = std::stoull(argv[4]);
  int32_t mode = 0;
  std::string m(argv[5]);
  if (m.find("read") != std::string::npos) {
    mode |= BENCHMARK_READ;
  }
  if (m.find("write") != std::string::npos) {
    mode |= BENCHMARK_WRITE;
  }
  if (m.find("destroy") != std::string::npos) {
    mode |= BENCHMARK_DESTROY;
  }
  size_t n_ops = std::stoull(argv[6]);
  bool warm_up = static_cast<bool>(std::stod(argv[7]));

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
  std::string output_prefix = result_prefix + "_" + std::to_string(value_size);
  uint64_t timeout = LAMBDA_TIMEOUT_SAFE;
  std::cerr << "Timeout: " << timeout << "us" << std::endl;
  if (!strcmp(argv[8], "zipf")) {
    auto begin = benchmark::now_us();
    auto key_gen = std::make_shared<zipf_key_generator>(0.0, n_ops);
    auto remaining = timeout - (benchmark::now_us() - begin);
    std::cerr << "Remaining: " << remaining << "us" << std::endl;
    benchmark::run(s_if, s_conf, key_gen, output_prefix, value_size, n_ops, warm_up, mode, remaining);
  } else if (!strcmp(argv[8], "sequential")) {
    auto begin = benchmark::now_us();
    auto key_gen = std::make_shared<sequential_key_generator>();
    auto remaining = timeout - (benchmark::now_us() - begin);
    std::cerr << "Remaining: " << remaining << "us" << std::endl;
    benchmark::run(s_if, s_conf, key_gen, output_prefix, value_size, n_ops, warm_up, mode, remaining);
  } else {
    std::cerr << "Unknown key distribution: " << argv[8] << std::endl;
  }

  return 0;
}