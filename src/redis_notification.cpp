#include <sstream>
#include "redis_notification.h"
#include "benchmark.h"

void redis_notification::init(const storage_interface::property_map &conf, bool) {
  std::string endpoint = conf.get<std::string>("endpoint", "127.0.0.1:6379");
  m_num_listeners = conf.get<size_t>("num_listeners");
  std::cerr << "num listeners = " << m_num_listeners << std::endl;
  m_notification_ts.resize(m_num_listeners);
  m_sub_msgs = new std::atomic<size_t>[m_num_listeners]();
  m_sub = std::make_shared<cpp_redis::subscriber>();
  m_client = std::make_shared<cpp_redis::client>();
  std::vector<std::string> endpoint_parts;
  benchmark_utils::split(endpoint, endpoint_parts, ':');
  m_sub->connect(endpoint_parts.front(), std::stoull(endpoint_parts.back()),
                 [](const std::string &host, std::size_t port, cpp_redis::subscriber::connect_state status) {
                   if (status == cpp_redis::subscriber::connect_state::dropped
                       || status == cpp_redis::subscriber::connect_state::failed
                       || status == cpp_redis::subscriber::connect_state::lookup_failed) {
                     std::cerr << "Redis client disconnected from " << host << ":" << port << std::endl;
                     exit(-1);
                   }
                 });

  m_client->connect(endpoint_parts.front(), std::stoull(endpoint_parts.back()),
                    [](const std::string &host, std::size_t port, cpp_redis::client::connect_state status) {
                      if (status == cpp_redis::client::connect_state::dropped
                          || status == cpp_redis::client::connect_state::failed
                          || status == cpp_redis::client::connect_state::lookup_failed) {
                        std::cerr << "Redis client disconnected from " << host << ":" << port << std::endl;
                        exit(-1);
                      }
                    });
}

void redis_notification::subscribe(const std::string &channel) {
  auto id = m_sub_count++;
  m_sub->subscribe(channel, [id, this](const std::string &, const std::string &) {
    std::cerr << "id = " << id << ", size = " << m_notification_ts.size() << std::endl;
    m_notification_ts[id].push_back(benchmark_utils::now_us());
    ++m_sub_msgs[id];
  });
  m_sub->commit();
}

void redis_notification::publish(const std::string &channel, const std::string &msg) {
  m_publish_ts.push_back(benchmark_utils::now_us());
  m_client->publish(channel, msg);
  m_client->commit();
}

std::vector<std::vector<uint64_t>> redis_notification::get_latencies() const {
  std::vector<std::vector<uint64_t>> ts(m_notification_ts.size());
  for (size_t i = 0; i < m_notification_ts.size(); ++i) {
    for (size_t j = 0; j < m_notification_ts[i].size(); ++j) {
      ts[i].push_back(m_notification_ts[i][j] - m_publish_ts[i]);
    }
  }
  return ts;
}

void redis_notification::destroy() {
  m_client->disconnect(true);
  m_sub->disconnect(true);
}

void redis_notification::wait(size_t i) {
  while (m_sub_msgs[i].load() != m_publish_ts.size());
}

REGISTER_NOTIFICATION_IFACE("redis", redis_notification);