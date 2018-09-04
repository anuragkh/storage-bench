#include "memorymux.h"
#include "mmux/directory/directory_ops.h"
void memorymux::init(const property_map &conf, bool create) {
  m_mmux_client = std::make_shared<mmux::client::mmux_client>(conf.get<std::string>("host", "127.0.0.1"),
                                                              conf.get<int>("service_port", 9090),
                                                              conf.get<int>("lease_port", 9091));
  m_mmux_path = conf.get<std::string>("path", "/test");
  if (m_mmux_path == "/test") {
    m_mmux_path += random_string(10);
    create = true;
  }
  if (create) {
    m_client = m_mmux_client->create(m_mmux_path, "local://tmp", 1, 1, mmux::directory::data_status::PINNED);
  } else {
    m_client = m_mmux_client->open(m_mmux_path);
  }
}

void memorymux::write(const std::string &key, const std::string &value) {
  auto resp = m_client->put(key, value);
  if (resp != "!ok") {
    throw std::runtime_error(resp);
  }
}

std::string memorymux::read(const std::string &key) {
  auto resp = m_client->get(key);
  if (resp.at(0) == '!') {
    throw std::runtime_error(resp);
  }
  return resp;
}

void memorymux::destroy() {
  m_client.reset();
  m_mmux_client->remove(m_mmux_path);
  m_mmux_client.reset();
}

void memorymux::write_async(const std::string &key, const std::string &value) {
  m_writes.push_back(key);
  m_writes.push_back(value);
}

void memorymux::read_async(const std::string &key) {
  m_reads.push_back(key);
}

void memorymux::wait_write() {
  if (!m_writes.empty()) {
    auto ret = m_client->put(m_writes);
    for (const auto &r: ret) {
      m_write_results.push(r);
    }
    m_writes.clear();
  }
  auto r = std::move(m_write_results.front());
  m_write_results.pop();
  if (r != "!ok") {
    throw std::runtime_error(r);
  }
}

std::string memorymux::wait_read() {
  if (!m_reads.empty()) {
    auto ret = m_client->get(m_reads);
    for (const auto &r: ret) {
      m_read_results.push(r);
    }
    m_reads.clear();
  }
  auto r = std::move(m_read_results.front());
  m_read_results.pop();
  return r;
}

REGISTER_STORAGE_IFACE("mmux", memorymux);
