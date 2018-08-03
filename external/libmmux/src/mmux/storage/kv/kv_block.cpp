#include "kv_block.h"

namespace mmux {
namespace storage {

std::vector<block_op> KV_OPS = {block_op{block_op_type::accessor, "exists"},
                                block_op{block_op_type::accessor, "get"},
                                block_op{block_op_type::accessor, "keys"},
                                block_op{block_op_type::accessor, "num_keys"},
                                block_op{block_op_type::mutator, "put"},
                                block_op{block_op_type::mutator, "remove"},
                                block_op{block_op_type::mutator, "update"},
                                block_op{block_op_type::mutator, "lock"},
                                block_op{block_op_type::mutator, "unlock"},
                                block_op{block_op_type::accessor, "locked_get_data_in_slot_range"},
                                block_op{block_op_type::accessor, "locked_get"},
                                block_op{block_op_type::mutator, "locked_put"},
                                block_op{block_op_type::mutator, "locked_remove"},
                                block_op{block_op_type::mutator, "locked_update"}};

}
}