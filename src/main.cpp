#include <iostream>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "storage_interface.h"
#include "benchmark.h"

#ifndef NUM_OPS
#define NUM_OPS 1000
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

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0] << " [system] [conf_file] [output_file_prefix] [benchmark_type]" << std::endl;
    return -1;
  }
  std::string system = argv[1];
  std::string conf_file = argv[2];
  std::string output_file_prefix = argv[3];
  std::string benchmark_type = argv[4];

  namespace pt = boost::property_tree;
  pt::ptree conf;
  try {
    pt::ini_parser::read_ini(conf_file, conf);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  auto iface = storage_interfaces::get_interface(system);
  std::vector<size_t> value_sizes;
  if (benchmark_type == "sweep") {
    for (size_t value_size = VALUE_SIZE_MIN; value_size <= VALUE_SIZE_MAX; value_size *= VALUE_SIZE_STEP) {
      value_sizes.push_back(value_size);
    }
  } else {
    value_sizes.push_back(std::stoull(benchmark_type));
  }

  for (auto value_size: value_sizes) {
    std::string benchmark_output_prefix = output_file_prefix + "_" + std::to_string(value_size);
    benchmark::run(iface, benchmark_output_prefix, value_size, NUM_OPS, conf.get_child(system));
  }

  return 0;
}