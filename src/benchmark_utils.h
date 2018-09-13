#ifndef STORAGE_BENCH_BENCHMARK_UTILS_H
#define STORAGE_BENCH_BENCHMARK_UTILS_H

#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <sstream>

class benchmark_utils {
 public:
  static void split(const std::string &str, std::vector<std::string> &cont, char delim = ' ') {
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
      cont.push_back(token);
    }
  }

  static bool time_bound(uint64_t start_us, uint64_t max_us) {
    if (now_us() - start_us < max_us)
      return true;
    std::cerr << "WARN Benchmark timed out..." << std::endl;
    return false;
  }

  static uint64_t now_us() {
    using namespace std::chrono;
    time_point<system_clock> now = system_clock::now();
    return static_cast<uint64_t>(duration_cast<microseconds>(now.time_since_epoch()).count());
  }

  static bool signal(const std::string &host, int port, const std::string &id) {
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
    struct hostent *he;
    struct in_addr **addr_list;
    if (inet_addr(host.c_str()) == INADDR_NONE) {
      if ((he = gethostbyname(host.c_str())) == nullptr) {
        // get the host info
        std::cerr << "Could not resolve hostname: " << host << std::endl;
        return false;
      }
      addr_list = (struct in_addr **) he->h_addr_list;
      server_address.sin_addr = *addr_list[0];
    } else {
      server_address.sin_addr.s_addr = inet_addr(host.c_str());
    }

    if (connect(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
      std::cerr << "Connection to " << host << ":" << port << " failed" << std::endl;
      return false;
    }

    std::string msg = "READY:" + id;
    std::cerr << "Signalling " << host << ":" << port << " with message " << msg << std::endl;
    ::send(sock, msg.data(), msg.length(), 0);

    char buffer[1024] = {0};
    ::read(sock, buffer, 1024);

    std::cerr << "buffer: [" << buffer << "]" << std::endl;
    return strcmp(buffer, "RUN") == 0;
  }
};

#endif //STORAGE_BENCH_BENCHMARK_UTILS_H
