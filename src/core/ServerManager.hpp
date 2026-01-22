#pragma once

#include <sys/types.h>

#include <map>
#include <vector>

#include "Connection.hpp"
#include "Server.hpp"

class ServerManager {
 private:
  ServerManager(const ServerManager& other);
  ServerManager& operator=(const ServerManager& other);

  int efd_;
  int sfd_;
  bool stop_requested_;
  std::map<int, Server> servers_;
  std::map<int, Connection> connections_;
  // Mapping of CGI pipe FDs to connection FDs for epoll event handling
  std::map<int, int> cgi_pipe_to_conn_;

  // Register a CGI pipe FD with epoll for monitoring
  // Returns true on success, false on error
  bool registerCgiPipe(int pipe_fd, int conn_fd);
  // Unregister a CGI pipe FD from epoll
  void unregisterCgiPipe(int pipe_fd);
  // Extract request body from connection's read buffer and validate
  // against Content-Length header if present.
  // Returns true when the full body is available (or no Content-Length).
  // Returns false when more data is required.
  bool extractRequestBody(Connection& conn);
  // Handle CGI pipe events (called when pipe is readable)
  void handleCgiPipeEvent(int pipe_fd);
  // Clean up handler resources (CGI pipes) for a connection before closing
  void cleanupHandlerResources(Connection& c);
  // Extract and validate request body from read buffer
  // Returns: 1 = body ready, 0 = need more data, -1 = error (response prepared)
  int extractRequestBody(Connection& conn, int conn_fd);
  // Prepare responses for connections that have completed reading
  // but do not yet have a write buffer
  void prepareResponses();
  // Check all connections for timeout and close stale ones
  void checkConnectionTimeouts();

 public:
  ServerManager();
  ~ServerManager();

  // Initializes all servers from configuration
  void initServers(std::vector<Server>& servers);

  // Accepts new client connection on given listening socket
  void acceptConnection(int listen_fd);

  // Main event loop: waits for events and handles requests
  int run();

  // Updates epoll events for a file descriptor
  void updateEvents(int fd, u_int32_t events);

  // Handle a single epoll event for fd with given event mask
  void handleEvent(int fd, u_int32_t ev_mask);

  void setupSignalHandlers();

  bool processSignalsFromFd();

  // Closes all connections and server sockets
  void shutdown();
};
