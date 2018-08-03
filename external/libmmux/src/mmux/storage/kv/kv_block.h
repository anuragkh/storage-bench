#ifndef MMUX_KV_SERVICE_SHARD_H
#define MMUX_KV_SERVICE_SHARD_H

#include <libcuckoo/cuckoohash_map.hh>

#include <string>
#include "serde/serde.h"
#include "serde/binary_serde.h"
#include "../block.h"
#include "kv_hash.h"
#include "serde/csv_serde.h"

namespace mmux {
namespace storage {

typedef std::string binary; // Since thrift translates binary to string
typedef binary key_type;
typedef binary value_type;

extern std::vector<block_op> KV_OPS;

enum kv_op_id : int32_t {
  exists = 0,
  get = 1,
  keys = 2, // TODO: We should not support multi-key operations since we do not provide any guarantees
  num_keys = 3, // TODO: We should not support multi-key operations since we do not provide any guarantees
  put = 4,
  remove = 5,
  update = 6,
  lock = 7,
  unlock = 8,
  locked_data_in_slot_range = 9,
  locked_get = 10,
  locked_put = 11,
  locked_remove = 12,
  locked_update = 13,
};

}
}

#endif //MMUX_KV_SERVICE_SHARD_H
