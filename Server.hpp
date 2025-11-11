#pragma once

#include "Location.hpp"
#include <map>
#include <string>
#include <vector>

class Server {
public:
  Server(void);
  Server(int port);
  Server(const Server &other);
  ~Server();

  Server &operator=(const Server &other);

  int fd;           // Server socket file descriptor
  int port;         // Port number to listen on
  unsigned int host; // Host address to bind to
  std::map<std::string, std::vector<std::string> >
      directives; // Server directives (root, index, autoindex, etc.)
  std::map<std::string, Location> locations; // Location blocks mapped by path

  // Initializes server socket, binds to port, and starts listening
  void init(void);

  // Closes server socket
  void disconnect(void);
};
