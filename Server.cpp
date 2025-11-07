#include "Server.hpp"
#include "Logger.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(void) : fd(-1), port(-1) {}

Server::Server(int port) : fd(-1), port(port) {}

Server::Server(const Server &other) : fd(other.fd), port(other.port) {}

Server::~Server() {
  disconnect();
}

Server &Server::operator=(const Server &other) {
  if (this != &other) {
    fd = other.fd;
    port = other.port;
  }
  return *this;
}

void Server::init(void) {
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    LOG_PERROR(ERROR, "socket");
    throw std::runtime_error("socket");
  }

  /* avoid "address already in use" on quick restarts */
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    disconnect();
    LOG_PERROR(ERROR, "setsockopt");
    throw std::runtime_error("setsockopt");
  }

  struct sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    disconnect();
    LOG_PERROR(ERROR, "bind");
    throw std::runtime_error("bind");
  }

  if (listen(fd, MAX_CONNECTIONS_PER_SERVER) < 0) {
    disconnect();
    LOG_PERROR(ERROR, "listen");
    throw std::runtime_error("listen");
  }

  if (set_nonblocking(fd) < 0) {
    disconnect();
    LOG_PERROR(ERROR, "set_nonblocking");
    throw std::runtime_error("set_nonblocking");
  }

  LOG(INFO) << "Server initialized on port " << port;
}

void Server::disconnect(void) {
  if (fd != -1) {
    close(fd);
    fd = -1;
  }
}
