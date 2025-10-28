#include "Server.hpp"
#include "constants.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server() {}

Server::Server(int port) : port(port) {
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    throw strerror(errno);
  }

  /* avoid "address already in use" on quick restarts */
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    close(fd);
    fd = -1;
    throw strerror(errno);
  }

  struct sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    fd = -1;
    throw strerror(errno);
  }

  if (listen(fd, MAX_CONNECTIONS_PER_SERVER) < 0) {
    close(fd);
    fd = -1;
    throw strerror(errno);
  }
}

Server::Server(const Server &other) : fd(other.fd), port(other.port) {}

Server::~Server() {
  if (fd != -1) {
    close(fd);
    fd = -1;
  }
}

Server &Server::operator=(Server other) {
  (void)other;
  return *this;
}
