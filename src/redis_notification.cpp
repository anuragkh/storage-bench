#include <sstream>
#include "redis_notification.h"
#include "benchmark_utils.h"

void redis_notification::init(const storage_interface::property_map &conf, bool) {
  std::string endpoint = conf.get<std::string>("endpoint", "127.0.0.1:6379");
  m_num_listeners = conf.get<size_t>("num_listeners");
  std::cerr << "num listeners = " << m_num_listeners << std::endl;
  m_notification_ts.resize(m_num_listeners);
  m_sub.resize(m_num_listeners);
  m_sub_msgs = new std::atomic<size_t>[m_num_listeners]();
  m_pub = std::make_shared<cpp_redis::client>();
  for (size_t i = 0; i < m_num_listeners; ++i) {
    m_sub[i] = std::make_shared<cpp_redis::subscriber>();
  }
  std::vector<std::string> endpoint_parts;
  benchmark_utils::split(endpoint, endpoint_parts, ':');
  m_host = endpoint_parts.front();
  m_port = std::stoull(endpoint_parts.back());
  m_pub->connect(m_host, m_port, [](const std::string &host, size_t port, cpp_redis::client::connect_state s) {
    if (s == cpp_redis::client::connect_state::dropped
        || s == cpp_redis::client::connect_state::failed
        || s == cpp_redis::client::connect_state::lookup_failed) {
      std::cerr << "Redis client disconnected from " << host << ":" << port << std::endl;
      exit(-1);
    }
  });
}

void redis_notification::subscribe(const std::string &channel) {
  auto id = m_sub_count++;
  m_sub[id]->connect(m_host, m_port, [](const std::string &host, size_t port, cpp_redis::subscriber::connect_state s) {
    if (s == cpp_redis::subscriber::connect_state::dropped
        || s == cpp_redis::subscriber::connect_state::failed
        || s == cpp_redis::subscriber::connect_state::lookup_failed) {
      std::cerr << "Redis client disconnected from " << host << ":" << port << std::endl;
      exit(-1);
    }
  });
  m_sub[id]->subscribe(channel, [id, this](const std::string &, const std::string &) {
    m_notification_ts[id].push_back(benchmark_utils::now_us());
    ++m_sub_msgs[id];
  }).commit();
}

void redis_notification::publish(const std::string &channel, const std::string &msg) {
  m_publish_ts.push_back(benchmark_utils::now_us());
  auto fut = m_pub->publish(channel, msg);
  m_pub->commit();
  auto reply = fut.get();
  if (reply.is_error()) {
    throw std::runtime_error("Publish failed: " + reply.error());
  }
}

void redis_notification::destroy() {
  m_pub->disconnect(true);
  for (auto &sub: m_sub) {
    sub->disconnect(true);
  }
}

void redis_notification::wait(size_t i) {
  while (m_sub_msgs[i].load() != m_publish_ts.size()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

REGISTER_NOTIFICATION_IFACE("redis", redis_notification);