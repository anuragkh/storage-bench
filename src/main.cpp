#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "storage_interface.h"
#include "benchmark.h"

#ifndef NUM_OPS
#define NUM_OPS 2000ULL
#endif

#ifndef MAX_DATASET_SIZE
#define MAX_DATASET_SIZE 2147483648
#endif

size_t num_ops(const std::string &system, size_t value_size) {
  if (system == "dynamodb") {
    return std::min(2 * NUM_OPS, static_cast<unsigned long long>(MAX_DATASET_SIZE / value_size));
  } else if (system == "redis" || system == "mmux") {
    return std::min(50 * NUM_OPS, static_cast<unsigned long long>(MAX_DATASET_SIZE / value_size));
  }
  return std::min(NUM_OPS, static_cast<unsigned long long>(MAX_DATASET_SIZE / value_size));
}

int main(int argc, char **argv) {
  if (argc != 8) {
    std::cerr << "Usage: " << argv[0] << " system conf_file output_prefix value_size mode num_ops warm_up" << std::endl;
    return -1;
  }
  std::string system = argv[1];
  std::string conf_file = argv[2];
  std::string result_prefix = argv[3];
  size_t value_size = std::stoull(argv[4]);
  int32_t mode = BENCHMARK_READ | BENCHMARK_WRITE;
  if (!strcmp(argv[5], "read")) {
    mode = BENCHMARK_READ;
  } else if (!strcmp(argv[5], "write")) {
    mode = BENCHMARK_WRITE;
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
  benchmark::run(s_if, s_conf, output_prefix, value_size, n_ops, warm_up, mode);

  return 0;
}