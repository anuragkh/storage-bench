#ifndef STORAGE_BENCH_MEMORYMUX_H
#define STORAGE_BENCH_MEMORYMUX_H

#include "storage_interface.h"

class memorymux: public storage_interface {
 public:
  void init(const property_map &conf) override;
  void write(const std::string &key, const std::string &value) override;
  std::string read(const std::string &key) override;
  void destroy() override;

 private:

};

#endif //STORAGE_BENCH_MEMORYMUX_H
