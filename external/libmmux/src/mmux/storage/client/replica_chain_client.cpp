#include <thrift/transport/TTransportException.h>
#include "replica_chain_client.h"
#include "../../utils/logger.h"
#include "../manager/detail/block_name_parser.h"
#include "../../utils/string_utils.h"

namespace mmux {
namespace storage {

using namespace utils;

replica_chain_client::replica_chain_client(std::shared_ptr<directory::directory_interface> fs,
                                           const std::string &path,
                                           const directory::replica_chain &chain,
                                           int timeout_ms) : fs_(fs), path_(path), in_flight_(false) {
  seq_.client_id = -1;
  seq_.client_seq_no = 0;
  connect(chain, timeout_ms);
  for (auto &op: KV_OPS) {
    cmd_client_.push_back(op.type == block_op_type::accessor ? &tail_ : &head_);
  }
}

replica_chain_client::~replica_chain_client() {
  disconnect();
}

void replica_chain_client::disconnect() {
  head_.disconnect();
  tail_.disconnect();
}

const directory::replica_chain &replica_chain_client::chain() const {
  return chain_;
}

void replica_chain_client::connect(const directory::replica_chain &chain, int timeout_ms) {
  chain_ = chain;
  timeout_ms_ = timeout_ms;
  auto h = block_name_parser::parse(chain_.block_names.front());
  head_.connect(h.host, h.service_port, h.id, timeout_ms);
  seq_.client_id = head_.get_client_id();
  if (chain_.block_names.size() == 1) {
    tail_ = head_;
  } else {
    auto t = block_name_parser::parse(chain_.block_names.back());
    tail_.connect(t.host, t.service_port, t.id, timeout_ms);
  }
  response_reader_ = tail_.get_command_response_reader(seq_.client_id);
  in_flight_ = false;
}

void replica_chain_client::send_command(int32_t cmd_id, const std::vector<std::string> &args) {
  if (in_flight_) {
    throw std::length_error("Cannot have more than one request in-flight");
  }
  cmd_client_.at(static_cast<unsigned long>(cmd_id))->command_request(seq_, cmd_id, args);
  in_flight_ = true;
}

std::vector<std::string> replica_chain_client::recv_response() {
  std::vector<std::string> ret;
  int64_t rseq = response_reader_.recv_response(ret);
  if (rseq != seq_.client_seq_no) {
    throw std::logic_error("SEQ: Expected=" + std::to_string(seq_.client_seq_no) + " Received=" + std::to_string(rseq));
  }
  seq_.client_seq_no++;
  in_flight_ = false;
  return ret;
}

std::vector<std::string> replica_chain_client::run_command(int32_t cmd_id, const std::vector<std::string> &args) {
  std::vector<std::string> response;
  bool retry = false;
  while (response.empty()) {
    try {
      send_command(cmd_id, args);
      response = recv_response();
      if (retry && response[0] == "!duplicate_key") {
        response[0] = "!ok";
      }
    } catch (apache::thrift::transport::TTransportException &e) {
      LOG(log_level::info) << "Error in connection to chain: " << e.what();
      connect(fs_->resolve_failures(path_, chain_), timeout_ms_);
      retry = true;
    }
  }
  return response;
}

std::vector<std::string> replica_chain_client::run_command_redirected(int32_t cmd_id,
                                                                      const std::vector<std::string> &args) {
  auto args_copy = args;
  args_copy.emplace_back("!redirected");
  send_command(cmd_id, args_copy);
  return recv_response();
}

std::shared_ptr<replica_chain_client::locked_client> replica_chain_client::lock() {
  return std::make_shared<replica_chain_client::locked_client>(*this);
}

bool replica_chain_client::is_connected() const {
  return head_.is_connected() && tail_.is_connected();
}

replica_chain_client::locked_client::locked_client(replica_chain_client &parent) : parent_(parent) {
  auto res = parent_.run_command(kv_op_id::lock, {});
  if (res[0] != "!ok") {
    redirecting_ = true;
    redirect_chain_ = utils::string_utils::split(res[0], '!');
  } else {
    redirecting_ = false;
  }
}

replica_chain_client::locked_client::~locked_client() {
  unlock();
}

void replica_chain_client::locked_client::unlock() {
  parent_.run_command(kv_op_id::unlock, {});
}

const directory::replica_chain &replica_chain_client::locked_client::chain() {
  return parent_.chain();
}

bool replica_chain_client::locked_client::redirecting() const {
  return redirecting_;
}

const std::vector<std::string> &replica_chain_client::locked_client::redirect_chain() {
  return redirect_chain_;
}

void replica_chain_client::locked_client::send_command(int32_t cmd_id, const std::vector<std::string> &args) {
  parent_.send_command(cmd_id, args);
}

std::vector<std::string> replica_chain_client::locked_client::recv_response() {
  return parent_.recv_response();
}

std::vector<std::string> replica_chain_client::locked_client::run_command(int32_t cmd_id,
                                                                          const std::vector<std::string> &args) {
  return parent_.run_command(cmd_id, args);
}

std::vector<std::string> replica_chain_client::locked_client::run_command_redirected(int32_t cmd_id,
                                                                                     const std::vector<std::string> &args) {
  return parent_.run_command_redirected(cmd_id, args);
}

}
}