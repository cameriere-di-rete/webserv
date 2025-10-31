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

  void initServers(const std::vector<int> &ports);

  void acceptConnection(int listen_fd);

  int run();

  void updateEvents(int fd, u_int32_t events);

  void shutdown();
};
