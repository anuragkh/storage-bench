#ifndef STORAGE_BENCH_MEMORYMUX_H
#define STORAGE_BENCH_MEMORYMUX_H

#include "storage_interface.h"
#include <mmux/client/mmux_client.h>

class memorymux: public storage_interface {
 public:
  void init(const property_map &conf, bool create) override;
  void write(const std::string &key, const std::string &value) override;
  std::string read(const std::string &key) override;
  void destroy() override;
  void write_async(const std::string &key, const std::string &value) override;
  void read_async(const std::string &key) override;
  void wait_write() override;
  std::string wait_read() override;

 private:
  std::vector<std::string> m_writes;
  std::vector<std::string> m_reads;
  std::queue<std::string> m_read_results;
  std::queue<std::string> m_write_results;
  std::string m_mmux_path;
  std::shared_ptr<mmux::client::mmux_client> m_mmux_client;
  std::shared_ptr<mmux::storage::kv_client> m_client;
};

#endif //STORAGE_BENCH_MEMORYMUX_H
