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

#ifndef VALUE_SIZE_MIN
#define VALUE_SIZE_MIN 8
#endif

#ifndef VALUE_SIZE_MAX
#define VALUE_SIZE_MAX 134217728
#endif

#ifndef VALUE_SIZE_STEP
#define VALUE_SIZE_STEP 4
#endif

size_t num_ops(const std::string& system, size_t value_size) {
  if (system == "dynamodb") {
    return std::min(2 * NUM_OPS, static_cast<unsigned long long>(MAX_DATASET_SIZE / value_size));
  } else if (system == "redis" || system == "mmux") {
    return std::min(50 * NUM_OPS, static_cast<unsigned long long>(MAX_DATASET_SIZE / value_size));
  }
  return std::min(NUM_OPS, static_cast<unsigned long long>(MAX_DATASET_SIZE / value_size));
}

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0] << " [system] [conf_file] [output_file_prefix] [value_size]" << std::endl;
    return -1;
  }
  std::string system = argv[1];
  std::string conf_file = argv[2];
  std::string output_file_prefix = argv[3];
  size_t value_size = std::stoull(argv[4]);

  namespace pt = boost::property_tree;
  pt::ptree conf;
  try {
    pt::ini_parser::read_ini(conf_file, conf);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  auto iface = storage_interfaces::get_interface(system);
  std::string benchmark_output_prefix = output_file_prefix + "_" + std::to_string(value_size);
  benchmark::run(iface, benchmark_output_prefix, value_size, num_ops(system, value_size), conf.get_child(system));

  return 0;
}