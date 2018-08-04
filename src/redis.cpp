#include "redis.h"

void redis::init(const storage_interface::property_map &conf) {
  m_client = std::make_shared<cpp_redis::client>();
  m_client->connect(conf.get<std::string>("host", "127.0.0.1"),
                    conf.get<size_t>("port", 6379),
                    [](const std::string &host, std::size_t port, cpp_redis::client::connect_state status) {
                      if (status == cpp_redis::client::connect_state::dropped
                          || status == cpp_redis::client::connect_state::failed
                          || status == cpp_redis::client::connect_state::lookup_failed) {
                        std::cerr << "Redis client disconnected from " << host << ":" << port << std::endl;
                        exit(-1);
                      }
                    });
}

void redis::write(const std::string &key, const std::string &value) {
  auto fut = m_client->set(key, value);
  m_client->sync_commit();
  auto reply = fut.get();
  if (reply.is_error()) {
    throw std::runtime_error(reply.error());
  }
}

std::string redis::read(const std::string &key) {
  auto fut = m_client->get(key);
  m_client->sync_commit();
  auto reply = fut.get();
  if (reply.is_error()) {
    throw std::runtime_error(reply.error());
  }
  auto value = reply.as_string();
  return value;
}

void redis::destroy() {
  auto fut = m_client->flushall();
  m_client->sync_commit();
  auto reply = fut.get();
  if (reply.is_error()) {
    std::cerr << "Failed to clear Redis" << std::endl;
    exit(-1);
  }
  m_client->disconnect(true);
}

REGISTER_STORAGE_IFACE("redis", redis);
