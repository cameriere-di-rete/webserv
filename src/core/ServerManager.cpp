#include "ServerManager.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "IHandler.hpp"
#include "Logger.hpp"
#include "constants.hpp"
#include "utils.hpp"

ServerManager::ServerManager() : efd_(-1), sfd_(-1), stop_requested_(false) {}

ServerManager::ServerManager(const ServerManager& other)
    : efd_(-1), sfd_(-1), stop_requested_(false) {
  (void)other;
}

ServerManager& ServerManager::operator=(const ServerManager& other) {
  (void)other;
  return *this;
}

ServerManager::~ServerManager() {
  LOG(DEBUG) << "Shutting down webserv...";
  shutdown();
}

void ServerManager::initServers(std::vector<Server>& servers) {
  LOG(DEBUG) << "Initializing " << servers.size() << " server(s)...";

  /* Check for duplicate listen addresses before initializing */
  std::set<std::pair<in_addr_t, int> > listen_addresses;
  for (std::vector<Server>::iterator it = servers.begin(); it != servers.end();
       ++it) {
    std::pair<in_addr_t, int> addr(it->host, it->port);
    if (listen_addresses.find(addr) != listen_addresses.end()) {
      LOG(ERROR) << "Duplicate listen address found: "
                 << inet_ntoa(*(in_addr*)&it->host) << ":" << it->port;
      throw std::runtime_error("Duplicate listen address in configuration");
    }
    listen_addresses.insert(addr);
  }

  for (std::vector<Server>::iterator it = servers.begin(); it != servers.end();
       ++it) {
    LOG(DEBUG) << "Initializing server on " << inet_ntoa(*(in_addr*)&it->host)
               << ":" << it->port;
    it->init();
    /* store by listening fd */
    servers_[it->fd] = *it;
    LOG(DEBUG) << "Server registered (" << inet_ntoa(*(in_addr*)&it->host)
               << ":" << it->port << ") with fd: " << it->fd;
    /* prevent server destructor from closing the fd of the temporary */
    it->fd = -1;
  }
  /* clear servers after moving them to ServerManager */
  servers.clear();
  LOG(DEBUG) << "All servers initialized successfully";
}

void ServerManager::acceptConnection(int listen_fd) {
  LOG(DEBUG) << "Accepting new connections on listen_fd: " << listen_fd;
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int conn_fd =
        accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
    if (conn_fd < 0) {
      LOG(DEBUG) << "accept returned error on fd: " << listen_fd
                 << " (stop accepting for now)";
      break;
    }

    LOG(DEBUG) << "New connection accepted (fd: " << conn_fd
               << ") from server fd: " << listen_fd;

    Connection connection(conn_fd);
    /* record which listening/server fd accepted this connection */
    connection.server_fd = listen_fd;
    connection.remote_addr = inet_ntoa(client_addr.sin_addr);
    connections_[conn_fd] = connection;

    // watch for reads; no write interest yet
    updateEvents(conn_fd, EPOLLIN);
    LOG(DEBUG) << "Connection fd " << conn_fd << " registered with EPOLLIN";
  }
}

void ServerManager::updateEvents(int fd, uint32_t events) {
  if (efd_ < 0) {
    LOG(ERROR) << "epoll fd not initialized";
    return;
  }

  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(efd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
    if (errno == ENOENT) {
      if (epoll_ctl(efd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        LOG_PERROR(ERROR, "epoll_ctl ADD");
        throw std::runtime_error("Failed to add file descriptor to epoll");
      }
    } else {
      LOG_PERROR(ERROR, "epoll_ctl MOD");
      throw std::runtime_error("Failed to modify epoll events");
    }
  }
}

int ServerManager::run() {
  LOG(DEBUG) << "Starting ServerManager event loop...";

  /* create epoll instance */
  efd_ = epoll_create1(0);
  if (efd_ < 0) {
    LOG_PERROR(ERROR, "epoll_create1");
    return EXIT_FAILURE;
  }
  LOG(DEBUG) << "Epoll instance created with fd: " << efd_;

  /* register listener fds */
  LOG(DEBUG) << "Registering " << servers_.size()
             << " server socket(s) with epoll";
  for (std::map<int, Server>::const_iterator it = servers_.begin();
       it != servers_.end(); ++it) {
    int listen_fd = it->first;
    struct epoll_event ev;
    ev.events = EPOLLIN; /* only need read events for the listener */
    ev.data.fd = listen_fd;
    if (epoll_ctl(efd_, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
      LOG_PERROR(ERROR, "epoll_ctl ADD listen_fd");
      return EXIT_FAILURE;
    }
    LOG(DEBUG) << "Registered listen_fd " << listen_fd << " with epoll";
  }

  /* register signalfd so signals are delivered as FD events */
  if (sfd_ < 0) {
    LOG(ERROR) << "signalfd not initialized";
    return EXIT_FAILURE;
  }
  struct epoll_event signal_ev;
  signal_ev.events = EPOLLIN;
  signal_ev.data.fd = sfd_;
  if (epoll_ctl(efd_, EPOLL_CTL_ADD, sfd_, &signal_ev) < 0) {
    LOG_PERROR(ERROR, "epoll_ctl ADD signalfd");
    return EXIT_FAILURE;
  }

  /* event loop */
  struct epoll_event events[MAX_EVENTS];
  LOG(DEBUG) << "Entering main event loop (waiting for connections)...";

  while (!stop_requested_) {
    // Use 1 second timeout to periodically check for CGI/connection timeouts
    // even when there are no I/O events
    int n = epoll_wait(efd_, events, MAX_EVENTS, 1000);
    if (n < 0) {
      if (errno == EINTR) {
        if (stop_requested_) {
          LOG(DEBUG)
              << "ServerManager: stop requested by signal, exiting event loop";
          break;
        }
        continue; /* interrupted by non-termination signal */
      }
      LOG_PERROR(ERROR, "epoll_wait");
      return EXIT_FAILURE;
    }

    LOG(DEBUG) << "epoll_wait returned " << n << " event(s)";

    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;
      uint32_t ev_mask = events[i].events;

      handleEvent(fd, ev_mask);

      if (stop_requested_) {
        return EXIT_SUCCESS;
      }
    }

    /* After processing events, iterate connections to prepare responses
       for those that completed reading but don't yet have a write buffer. */
    LOG(DEBUG) << "Checking " << connections_.size()
               << " connection(s) for response preparation";

    prepareResponses();

    // Check for timed out connections AFTER processing all events.
    // This ensures connections with pending EPOLLIN/EPOLLOUT events get a
    // chance to update their activity timestamp before being checked.
    checkConnectionTimeouts();
  }
  LOG(DEBUG) << "ServerManager: exiting event loop";
  return EXIT_SUCCESS;
}

bool ServerManager::extractRequestBody(Connection& conn) {
  // Extract the request body from read_buffer (after \r\n\r\n)
  // The body starts at headers_end_pos + 4 (length of "\r\n\r\n")
  std::size_t body_start = conn.headers_end_pos + 4;
  if (body_start < conn.read_buffer.size()) {
    conn.request.getBody().data = conn.read_buffer.substr(body_start);
    LOG(DEBUG) << "Extracted request body: "
               << conn.request.getBody().data.size() << " bytes";
  } else {
    conn.request.getBody().data.clear();
  }

  // If a Content-Length header is present, wait until we've received the
  // entire body before processing the request. Otherwise we may switch
  // the socket to EPOLLOUT and stop reading, causing large uploads to
  // stall when the remaining body hasn't been consumed yet.
  std::string content_length_str;
  if (conn.request.getHeader("Content-Length", content_length_str)) {
    long long content_len = 0;
    if (safeStrtoll(content_length_str, content_len)) {
      if (conn.request.getBody().data.size() <
          static_cast<std::size_t>(content_len)) {
        LOG(DEBUG) << "Waiting for full request body: have "
                   << conn.request.getBody().data.size() << " of "
                   << content_len << " bytes";
        return false;
      }
    }
  }
  return true;
}

void ServerManager::setupSignalHandlers() {
  // Block the signals we want to handle
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);

  if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
    LOG_PERROR(ERROR, "sigprocmask");
    throw std::runtime_error("Failed to block signals with sigprocmask");
  }

  // Create signalfd - REQUIRED: signalfd must be available (Linux 2.6.22+).
  // If signalfd is not available, initialization will fail.
  sfd_ = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
  if (sfd_ < 0) {
    LOG_PERROR(ERROR, "signalfd");
    // Unblock the signals before throwing, to avoid leaving the process in a
    // bad state
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    throw std::runtime_error("Failed to create signalfd");
  }

  // Ignore SIGPIPE
  struct sigaction sa_pipe;
  std::memset(&sa_pipe, 0, sizeof(sa_pipe));
  sa_pipe.sa_handler = SIG_IGN;
  sigemptyset(&sa_pipe.sa_mask);
  if (sigaction(SIGPIPE, &sa_pipe, NULL) < 0) {
    LOG_PERROR(ERROR, "sigaction(SIGPIPE)");
    throw std::runtime_error("Failed to ignore SIGPIPE with sigaction");
  }

  LOG(DEBUG) << "signals: signalfd installed and signals blocked";
}

bool ServerManager::processSignalsFromFd() {
  struct signalfd_siginfo fdsi;
  while (true) {
    ssize_t s = read(sfd_, &fdsi, sizeof(fdsi));
    if (s < 0) {
      LOG_PERROR(ERROR, "read(signalfd)");
      return stop_requested_;
    }
    if (s == 0) {
      LOG(ERROR) << "signals: signalfd closed unexpectedly";
      return stop_requested_;
    }
    if (s != sizeof(fdsi)) {
      LOG(ERROR) << "signals: partial read from signalfd (" << s
                 << " bytes, expected " << sizeof(fdsi) << ")";
      return stop_requested_;
    }

    // Handle the signal
    if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM) {
      stop_requested_ = true;
      return true;
    }
    LOG(DEBUG) << "signals: got unexpected signo=" << fdsi.ssi_signo;
  }
}

void ServerManager::shutdown() {
  LOG(INFO) << "Shutting down webserv...";

  if (efd_ >= 0) {
    LOG(DEBUG) << "Closing epoll fd: " << efd_;
    close(efd_);
    efd_ = -1;
  }

  if (sfd_ >= 0) {
    LOG(DEBUG) << "Closing signalfd: " << sfd_;
    close(sfd_);
    sfd_ = -1;
  }

  // close all connection fds
  LOG(DEBUG) << "Closing " << connections_.size() << " connection(s)";
  for (std::map<int, Connection>::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    close(it->first);
  }
  connections_.clear();

  // Clear CGI pipe mappings (pipes are owned by handlers which are cleaned up
  // by connections)
  cgi_pipe_to_conn_.clear();

  // close listening fds
  LOG(DEBUG) << "Closing " << servers_.size() << " server socket(s)";
  for (std::map<int, Server>::iterator it = servers_.begin();
       it != servers_.end(); ++it) {
    it->second.disconnect();
  }
  servers_.clear();

  LOG(INFO) << "webserv shutdown complete";
}

bool ServerManager::registerCgiPipe(int pipe_fd, int conn_fd) {
  cgi_pipe_to_conn_[pipe_fd] = conn_fd;

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = pipe_fd;

  if (epoll_ctl(efd_, EPOLL_CTL_ADD, pipe_fd, &ev) < 0) {
    LOG_PERROR(ERROR, "epoll_ctl ADD CGI pipe");
    cgi_pipe_to_conn_.erase(pipe_fd);
    return false;
  }
  return true;
}

void ServerManager::unregisterCgiPipe(int pipe_fd) {
  std::map<int, int>::iterator it = cgi_pipe_to_conn_.find(pipe_fd);
  if (it == cgi_pipe_to_conn_.end()) {
    return;
  }

  if (efd_ >= 0) {
    epoll_ctl(efd_, EPOLL_CTL_DEL, pipe_fd, NULL);
  }
  cgi_pipe_to_conn_.erase(it);
}

void ServerManager::handleCgiPipeEvent(int pipe_fd) {
  std::map<int, int>::iterator cgi_it = cgi_pipe_to_conn_.find(pipe_fd);
  if (cgi_it == cgi_pipe_to_conn_.end()) {
    LOG(ERROR) << "CGI pipe fd " << pipe_fd << " not found in mapping";
    return;
  }

  int conn_fd = cgi_it->second;
  std::map<int, Connection>::iterator c_it = connections_.find(conn_fd);
  if (c_it == connections_.end()) {
    LOG(ERROR) << "Connection fd " << conn_fd << " not found for CGI pipe "
               << pipe_fd;
    unregisterCgiPipe(pipe_fd);
    return;
  }

  Connection& conn = c_it->second;
  if (conn.active_handler == NULL) {
    LOG(ERROR) << "No active handler for connection fd " << conn_fd;
    unregisterCgiPipe(pipe_fd);
    return;
  }

  // Resume the handler to read more CGI output
  HandlerResult hr = conn.active_handler->resume(conn);

  if (hr == HR_WOULD_BLOCK) {
    // More data expected, keep monitoring the pipe
    LOG(DEBUG) << "CGI handler would block, continuing to monitor pipe fd "
               << pipe_fd;
    return;
  }

  // CGI finished (HR_DONE) or error (HR_ERROR)
  unregisterCgiPipe(pipe_fd);

  if (hr == HR_ERROR) {
    LOG(ERROR) << "CGI handler error on connection fd " << conn_fd;
    conn.clearHandler();
    conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
  } else {
    // HR_DONE - CGI completed successfully
    LOG(DEBUG) << "CGI handler completed for connection fd " << conn_fd;
    conn.clearHandler();
  }

  // Enable write events to send response
  updateEvents(conn_fd, EPOLLOUT);
}

void ServerManager::prepareResponses() {
  for (std::map<int, Connection>::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    Connection& conn = it->second;
    int conn_fd = it->first;

    if (conn.headers_end_pos == std::string::npos) {
      continue;
    }

    if (!conn.write_buffer.empty()) {
      continue; /* already prepared */
    }

    // Skip if handler is already active (e.g., waiting for CGI output)
    if (conn.active_handler != NULL) {
      continue;
    }

    LOG(DEBUG) << "Preparing response for connection fd: " << conn_fd;

    if (!conn.request.parseStartAndHeaders(conn.read_buffer,
                                           conn.headers_end_pos)) {
      /* malformed start line or headers -> 400 Bad Request */
      LOG(INFO) << "Malformed request on fd " << conn_fd
                << ", sending 400 Bad Request";
      conn.prepareErrorResponse(http::S_400_BAD_REQUEST);
      updateEvents(conn_fd, EPOLLOUT);
      continue;
    }

    // Extract request body & check Content-Length via helper
    if (!extractRequestBody(conn)) {
      // Not all body bytes received yet — keep monitoring for reads
      continue;
    }

    /* find the server that accepted this connection */
    std::map<int, Server>::iterator srv_it = servers_.find(conn.server_fd);
    if (srv_it == servers_.end()) {
      /* shouldn't happen, but handle gracefully */
      LOG(ERROR) << "Server not found for connection fd " << conn_fd
                 << " (server_fd: " << conn.server_fd << ")";
      conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
      updateEvents(conn_fd, EPOLLOUT);
      continue;
    }

    LOG(DEBUG) << "Found server configuration for fd " << conn_fd
               << " (port: " << srv_it->second.port << ")";

    /* process request using new handler methods */
    conn.processRequest(srv_it->second);

    // Check if handler needs async I/O (e.g., CGI pipe monitoring)
    if (conn.active_handler != NULL) {
      int monitor_fd = conn.active_handler->getMonitorFd();
      if (monitor_fd >= 0) {
        // Register CGI pipe for epoll monitoring
        LOG(DEBUG) << "Registering CGI pipe fd " << monitor_fd
                   << " for connection fd " << conn_fd;
        if (!registerCgiPipe(monitor_fd, conn_fd)) {
          // Failed to register pipe, send 500 error
          LOG(ERROR) << "Failed to register CGI pipe for connection fd "
                     << conn_fd;
          conn.clearHandler();
          conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
          updateEvents(conn_fd, EPOLLOUT);
          continue;
        }
        // Don't enable EPOLLOUT yet - wait for CGI to complete
        continue;
      }
    }

    /* enable EPOLLOUT now that we have data to send */
    updateEvents(conn_fd, EPOLLOUT);
  }
}

void ServerManager::handleEvent(int fd, u_int32_t ev_mask) {
  LOG(DEBUG) << "Processing event for fd: " << fd;

  if (fd == sfd_) {
    // process pending signals from signalfd
    if (processSignalsFromFd()) {
      LOG(DEBUG) << "ServerManager: stop requested by signal (signalfd)";
    }
    return;
  }

  std::map<int, Server>::iterator s_it = servers_.find(fd);
  if (s_it != servers_.end()) {
    LOG(DEBUG) << "Event is on server listen socket, accepting connections...";
    acceptConnection(fd);
    return;
  }

  // Check if this is a CGI pipe FD
  std::map<int, int>::iterator cgi_it = cgi_pipe_to_conn_.find(fd);
  if (cgi_it != cgi_pipe_to_conn_.end()) {
    LOG(DEBUG) << "EPOLLIN event on CGI pipe fd: " << fd;
    handleCgiPipeEvent(fd);
    return;
  }

  std::map<int, Connection>::iterator c_it = connections_.find(fd);
  if (c_it == connections_.end()) {
    LOG(DEBUG) << "Unknown fd: " << fd << ", skipping";
    return; /* unknown fd */
  }

  Connection& c = c_it->second;

  /* readable */
  if (ev_mask & EPOLLIN) {
    LOG(DEBUG) << "EPOLLIN event on connection fd: " << fd;
    int status = c.handleRead();

    if (status < 0) {
      LOG(DEBUG) << "handleRead failed, closing connection fd: " << fd;
      cleanupHandlerResources(c);
      close(fd);
      connections_.erase(fd);
      return;
    }

    if (c.headers_end_pos != std::string::npos) {
      LOG(DEBUG) << "Headers complete on fd: " << fd;
    }
  }

  /* writable */
  if (ev_mask & EPOLLOUT) {
    LOG(DEBUG) << "EPOLLOUT event on connection fd: " << fd;
    int status = c.handleWrite();

    if (status <= 0) {
      // Log the completed request in nginx-style format
      c.logAccess();
      LOG(DEBUG) << "handleWrite complete or failed, closing connection fd: "
                 << fd;
      cleanupHandlerResources(c);
      close(fd);
      connections_.erase(fd);
    }
  }
}

void ServerManager::cleanupHandlerResources(Connection& c) {
  if (c.active_handler != NULL) {
    int monitor_fd = c.active_handler->getMonitorFd();
    if (monitor_fd >= 0) {
      unregisterCgiPipe(monitor_fd);
    }
  }
}

void ServerManager::checkConnectionTimeouts() {
  std::vector<int> timed_out_fds;
  std::vector<int> cgi_timed_out_fds;

  // First pass: identify timed out connections
  for (std::map<int, Connection>::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    Connection& conn = it->second;
    int conn_fd = it->first;

    // Check for CGI handler timeouts first
    if (conn.active_handler != NULL &&
        conn.active_handler->checkTimeout(conn)) {
      LOG(INFO) << "CGI timeout on fd " << conn_fd;
      cgi_timed_out_fds.push_back(conn_fd);
      continue;
    }

    // Check for read phase timeouts (connections waiting for client data)
    if (conn.isReadTimedOut(READ_TIMEOUT_SECONDS)) {
      LOG(INFO) << "Read timeout on fd " << conn_fd
                << " (idle for >= " << READ_TIMEOUT_SECONDS << "s)";
      timed_out_fds.push_back(conn_fd);
      continue;
    }

    // Check for write phase timeouts (connections stuck sending responses)
    if (conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS)) {
      LOG(INFO) << "Write timeout on fd " << conn_fd
                << " (sending for >= " << WRITE_TIMEOUT_SECONDS << "s)";
      timed_out_fds.push_back(conn_fd);
    }
  }

  // Handle CGI timeouts - cleanup handler and send response
  for (std::size_t i = 0; i < cgi_timed_out_fds.size(); ++i) {
    int conn_fd = cgi_timed_out_fds[i];
    std::map<int, Connection>::iterator it = connections_.find(conn_fd);
    if (it == connections_.end()) {
      continue;
    }

    Connection& conn = it->second;

    // Unregister CGI pipe if any
    if (conn.active_handler != NULL) {
      int pipe_fd = conn.active_handler->getMonitorFd();
      if (pipe_fd >= 0) {
        unregisterCgiPipe(pipe_fd);
      }
    }

    // Clear the CGI handler first, then prepare error response
    // This order is important to avoid use-after-free when
    // prepareErrorResponse installs a new handler (ErrorFileHandler)
    conn.clearHandler();
    conn.prepareErrorResponse(http::S_504_GATEWAY_TIMEOUT);
    updateEvents(conn_fd, EPOLLOUT);
  }

  // Second pass: close timed out connections
  for (std::size_t i = 0; i < timed_out_fds.size(); ++i) {
    int conn_fd = timed_out_fds[i];
    std::map<int, Connection>::iterator it = connections_.find(conn_fd);
    if (it == connections_.end()) {
      continue;  // Already removed
    }

    Connection& conn = it->second;

    // Only send 408 if:
    // 1. No response has been prepared yet (write_buffer is empty)
    // 2. No response is in progress (status_code is still unknown)
    // This prevents overwriting a partially sent response with a 408 error
    if (conn.write_buffer.empty() &&
        conn.response.status_line.status_code == http::S_0_UNKNOWN) {
      conn.prepareErrorResponse(http::S_408_REQUEST_TIMEOUT);
      // Update epoll events to watch for EPOLLOUT so the response is sent
      // in the next event loop iteration. This is more reliable than
      // attempting to send immediately, as the socket might not be ready.
      updateEvents(conn_fd, EPOLLOUT);
      continue;  // Skip cleanup and closing, let event loop handle it
    }

    // Clean up and close
    cleanupHandlerResources(conn);
    close(conn_fd);
    connections_.erase(conn_fd);
  }
}
