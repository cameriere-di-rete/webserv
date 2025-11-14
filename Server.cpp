#include "Server.hpp"
#include "Logger.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(void)
    : fd(-1), port(-1), host(INADDR_ANY), allow_methods(), index(),
      autoindex(false), root(), error_page(), max_request_body(0), locations() {
  LOG(DEBUG) << "Server() default constructor called";
  // Default allowed methods
  allow_methods.insert(http::GET);
  allow_methods.insert(http::POST);
  allow_methods.insert(http::PUT);
  allow_methods.insert(http::DELETE);
  allow_methods.insert(http::HEAD);
  LOG(DEBUG) << "Server initialized with default allowed methods";
}

Server::Server(int port)
    : fd(-1), port(port), host(INADDR_ANY), allow_methods(), index(),
      autoindex(false), root(), error_page(), max_request_body(0), locations() {
  LOG(DEBUG) << "Server(port) constructor called with port: " << port;
  // Default allowed methods
  allow_methods.insert(http::GET);
  allow_methods.insert(http::POST);
  allow_methods.insert(http::PUT);
  allow_methods.insert(http::DELETE);
  allow_methods.insert(http::HEAD);
  LOG(DEBUG) << "Server on port " << port
             << " initialized with default allowed methods";
}

Server::Server(const Server &other)
    : fd(other.fd), port(other.port), host(other.host),
      allow_methods(other.allow_methods), index(other.index),
      autoindex(other.autoindex), root(other.root),
      error_page(other.error_page), max_request_body(other.max_request_body),
      locations(other.locations) {}

Server::~Server() {
  disconnect();
}

Server &Server::operator=(const Server &other) {
  if (this != &other) {
    fd = other.fd;
    port = other.port;
    host = other.host;
    allow_methods = other.allow_methods;
    index = other.index;
    autoindex = other.autoindex;
    root = other.root;
    error_page = other.error_page;
    max_request_body = other.max_request_body;
    locations = other.locations;
  }
  return *this;
}

void Server::init(void) {
  LOG(INFO) << "Initializing server on " << inet_ntoa(*(in_addr *)&host) << ":"
            << port << "...";

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    LOG_PERROR(ERROR, "socket");
    throw std::runtime_error("socket");
  }
  LOG(DEBUG) << "Socket created with fd: " << fd;

  /* avoid "address already in use" on quick restarts */
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    disconnect();
    LOG_PERROR(ERROR, "setsockopt");
    throw std::runtime_error("setsockopt");
  }
  LOG(DEBUG) << "SO_REUSEADDR option set on socket";

  struct sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  /* bind to configured host (INADDR_ANY if not set) */
  addr.sin_addr.s_addr = host;
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    disconnect();
    LOG_PERROR(ERROR, "bind");
    throw std::runtime_error("bind");
  }
  LOG(DEBUG) << "Socket bound to " << inet_ntoa(*(in_addr *)&host) << ":"
             << port;

  if (listen(fd, MAX_CONNECTIONS_PER_SERVER) < 0) {
    disconnect();
    LOG_PERROR(ERROR, "listen");
    throw std::runtime_error("listen");
  }
  LOG(DEBUG) << "Socket listening with backlog: " << MAX_CONNECTIONS_PER_SERVER;

  if (set_nonblocking(fd) < 0) {
    disconnect();
    LOG_PERROR(ERROR, "set_nonblocking");
    throw std::runtime_error("set_nonblocking");
  }
  LOG(DEBUG) << "Socket set to non-blocking mode";

  LOG(INFO) << "Server successfully initialized on port " << port
            << " (fd: " << fd << ")";
}

void Server::disconnect(void) {
  if (fd != -1) {
    LOG(DEBUG) << "Closing server socket fd: " << fd;
    close(fd);
    fd = -1;
  }
}
