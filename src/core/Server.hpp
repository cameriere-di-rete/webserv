#pragma once

#include <netinet/in.h>

#include <set>
#include <string>

#include "../config/Location.hpp"

class Server {
 public:
  Server(void);
  Server(int port);
  Server(const Server& other);
  ~Server();

  Server& operator=(const Server& other);

  int fd;
  int port;
  in_addr_t host;

  std::set<http::Method> allow_methods;
  std::set<std::string> index;
  bool autoindex;
  std::string root;
  std::map<http::Status, std::string> error_page;
  std::size_t max_request_body;

  std::map<std::string, Location> locations;

  void init(void);
  void disconnect(void);
  Location matchLocation(const std::string& path) const;
};
