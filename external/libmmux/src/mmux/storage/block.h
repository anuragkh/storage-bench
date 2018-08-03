#ifndef MMUX_BLOCK_H
#define MMUX_BLOCK_H

#include <utility>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#define MAX_BLOCK_OP_NAME_SIZE 63

namespace mmux {
namespace storage {

enum block_op_type : uint8_t {
  accessor = 0,
  mutator = 1
};

enum block_state {
  regular = 0,
  importing = 1,
  exporting = 2
};

struct block_op {
  block_op_type type;
  char name[MAX_BLOCK_OP_NAME_SIZE];

  bool operator<(const block_op &other) const {
    return std::strcmp(name, other.name) < 0;
  }
};

class block {
 public:
  static const int32_t SLOT_MAX = 65536;
};

}
}
#endif //MMUX_BLOCK_H
