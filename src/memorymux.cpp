#include "memorymux.h"

void memorymux::init(const storage_interface::property_map &conf) {
  m_mmux_client = std::make_shared<mmux::client::mmux_client>(conf.get<std::string>("host", "127.0.0.1"),
                                                              conf.get<int>("service_port", 9090),
                                                              conf.get<int>("lease_port", 9091));
  m_mmux_path = conf.get<std::string>("path", "/test");
  m_client = m_mmux_client->open_or_create(m_mmux_path, "local://tmp");
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
  throw std::runtime_error("Not supported");
}

void memorymux::read_async(const std::string &key) {
  throw std::runtime_error("Not supported");
}

void memorymux::wait_writes() {
  throw std::runtime_error("Not supported");
}

void memorymux::wait_reads(std::vector<std::string> &results) {
  throw std::runtime_error("Not supported");
}

REGISTER_STORAGE_IFACE("mmux", memorymux);
