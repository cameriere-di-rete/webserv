#pragma once

#include "Location.hpp"
#include <map>
#include <netinet/in.h>
#include <set>
#include <string>

class Server {
public:
  Server(void);
  Server(int port);
  Server(const Server &other);
  ~Server();

  Server &operator=(const Server &other);

  // Socket and network configuration
  int fd;         // Server socket file descriptor
  int port;       // Port number to listen on
  in_addr_t host; // Host address to bind to

  // Server-level configuration (inherited by locations unless overridden)
  std::set<Location::Method> allow_methods;
  std::set<std::string> index;
  bool autoindex;
  std::string root;
  std::map<int, std::string> error_page;
  std::size_t max_request_body;

  // Location blocks mapped by path
  std::map<std::string, Location> locations;

  // Initializes server socket, binds to port, and starts listening
  void init(void);

  // Closes server socket
  void disconnect(void);
};
