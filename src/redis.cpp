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
  auto fut = send_write(key, value);
  parse_write_response(fut.get());
}

std::string redis::read(const std::string &key) {
  auto fut = send_read(key);
  return parse_read_response(fut.get());
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

void redis::write_async(const std::string &key, const std::string &value) {
  m_put_futures.push_back(send_write(key, value));
}

void redis::read_async(const std::string &key) {
  m_get_futures.push_back(send_read(key));
}

void redis::wait_writes() {
  for (auto &fut: m_put_futures) {
    parse_write_response(fut.get());
  }
  m_put_futures.clear();
}

void redis::wait_reads(std::vector<std::string> &results) {
  for (auto &fut: m_get_futures) {
    results.push_back(parse_read_response(fut.get()));
  }
  m_get_futures.clear();
}

std::future<cpp_redis::reply> redis::send_write(const std::string &key, const std::string &value) {
  auto fut = m_client->set(key, value);
  m_client->commit();
  return fut;
}

std::future<cpp_redis::reply> redis::send_read(const std::string &key) {
  auto fut = m_client->get(key);
  m_client->commit();
  return fut;
}

std::string redis::parse_read_response(const cpp_redis::reply &r) {
  if (r.is_error()) {
    throw std::runtime_error(r.error());
  }
  return r.as_string();
}

void redis::parse_write_response(const cpp_redis::reply &r) {
  if (r.is_error()) {
    throw std::runtime_error(r.error());
  }
}

REGISTER_STORAGE_IFACE("redis", redis);
