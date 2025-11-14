#pragma once

#include "Connection.hpp"
#include "Server.hpp"
#include <map>
#include <sys/types.h>
#include <vector>

class ServerManager {
private:
  ServerManager(const ServerManager &other);
  ServerManager &operator=(const ServerManager &other);

  int _efd;
  std::map<int, Server> _servers;
  std::map<int, Connection> _connections;

public:
  ServerManager();
  ~ServerManager();

  // Initializes all servers from configuration
  void initServers(std::vector<Server> &servers);

  // Accepts new client connection on given listening socket
  void acceptConnection(int listen_fd);

  // Main event loop: waits for events and handles requests
  int run();

  // Updates epoll events for a file descriptor
  void updateEvents(int fd, u_int32_t events);

  // Closes all connections and server sockets
  void shutdown();
};
