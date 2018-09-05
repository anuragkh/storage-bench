#ifndef STORAGE_BENCH_STORAGE_INTERFACE_H
#define STORAGE_BENCH_STORAGE_INTERFACE_H

#include <string>
#include <map>
#include <utility>
#include <memory>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

class storage_interface {
 public:
  typedef boost::property_tree::ptree property_map;

  bool signal(const std::string &host, int port, const std::string &id) {
    int sock = 0;
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      std::cerr << "Socket creation error" << std::endl;
      return false;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, host.c_str(), &server_address.sin_addr) <= 0) {
      std::cerr << "Invalid address/address not supported" << std::endl;
      return false;
    }

    if (connect(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
      std::cerr << "Connection failed" << std::endl;
      return false;
    }

    std::string msg = "READY:" + id;
    ::send(sock, msg.data(), msg.length(), 0);

    char buffer[1024] = {0};
    ::read(sock, buffer, 1024);

    std::cerr << "buffer: [" << buffer << "]" << std::endl;
    return strcmp(buffer, "RUN") == 0;
  }

  virtual void init(const property_map &conf, bool create) = 0;
  virtual void write(const std::string &key, const std::string &value) = 0;
  virtual std::string read(const std::string &key) = 0;
  virtual void destroy() = 0;

  virtual void write_async(const std::string &key, const std::string &value) = 0;
  virtual void read_async(const std::string &key) = 0;

  virtual void wait_write() = 0;
  virtual std::string wait_read() = 0;

  static std::string random_string(size_t length);
};

class storage_interfaces {
 public:
  typedef std::map<std::string, std::shared_ptr<storage_interface>> interface_map;

  static void register_interface(const std::string &name, std::shared_ptr<storage_interface> iface) {
    interfaces()->insert({name, iface});
  }

  static void deregister_interface(const std::string &name) {
    interfaces()->erase(name);
  }

  static std::shared_ptr<storage_interface> get_interface(const std::string &name) {
    auto it = interfaces()->find(name);
    if (it == interfaces()->end()) {
      throw std::invalid_argument("No such interface " + name);
    }
    return it->second;
  }

  static std::shared_ptr<interface_map> interfaces() {
    if (m_interface_map == nullptr) {
      m_interface_map = std::make_shared<interface_map>();
    }
    return m_interface_map;
  };

 private:
  static std::shared_ptr<interface_map> m_interface_map;
};

#define REGISTER_STORAGE_IFACE(name, iface)                                     \
  class iface##_class {                                                         \
   public:                                                                      \
    iface##_class() {                                                           \
      storage_interfaces::register_interface(name, std::make_shared<iface>());  \
    }                                                                           \
    ~iface##_class() {                                                          \
      storage_interfaces::deregister_interface(name);                           \
    }                                                                           \
  };                                                                            \
  static iface##_class iface##_singleton

#endif //STORAGE_BENCH_STORAGE_INTERFACE_H