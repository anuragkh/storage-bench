#ifndef MMUX_LEASE_RENEWAL_WORKER_H
#define MMUX_LEASE_RENEWAL_WORKER_H

#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

#include "lease_client.h"

namespace mmux {
namespace directory {

class lease_renewal_worker {
 public:
  lease_renewal_worker(const std::string &host, int port);
  ~lease_renewal_worker();

  void start();
  void stop();

  void add_path(const std::string &path);
  void remove_path(const std::string &path);
  bool has_path(const std::string &path);
 private:
  mutable std::mutex metadata_mtx_;

  std::thread worker_;
  std::atomic_bool stop_;
  lease_client ls_;
  std::vector<std::string> to_renew_;
};

}
}

#endif //MMUX_LEASE_RENEWAL_WORKER_H
