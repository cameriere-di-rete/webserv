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
  LOG(DEBUG) << "Shutting down ServerManager...";
  shutdown();
}

void ServerManager::initServers(std::vector<Server>& servers) {
  LOG(INFO) << "Initializing " << servers.size() << " server(s)...";

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
  LOG(INFO) << "All servers initialized successfully";
}

void ServerManager::acceptConnection(int listen_fd) {
  LOG(DEBUG) << "Accepting new connections on listen_fd: " << listen_fd;
  while (1) {
    int conn_fd = accept(listen_fd, NULL, NULL);
    if (conn_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        LOG(DEBUG) << "No more pending connections on fd: " << listen_fd;
        break;
      }
      LOG_PERROR(ERROR, "accept");
      break;
    }
    if (set_nonblocking(conn_fd) < 0) {
      LOG_PERROR(ERROR, "set_nonblocking conn_fd");
      close(conn_fd);
      continue;
    }

    LOG(INFO) << "New connection accepted (fd: " << conn_fd
              << ") from server fd: " << listen_fd;

    Connection connection(conn_fd);
    /* record which listening/server fd accepted this connection */
    connection.server_fd = listen_fd;
    connections_[conn_fd] = connection;

    // watch for reads; no write interest yet
    updateEvents(conn_fd, EPOLLIN | EPOLLET);
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
  LOG(INFO) << "Starting ServerManager event loop...";

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
  LOG(INFO) << "Entering main event loop (waiting for connections)...";

  while (!stop_requested_) {
    int n = epoll_wait(efd_, events, MAX_EVENTS, -1);
    if (n < 0) {
      if (errno == EINTR) {
        if (stop_requested_) {
          LOG(INFO)
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
      LOG(DEBUG) << "Processing event for fd: " << fd;

      if (fd == sfd_) {
        // process pending signals from signalfd
        if (processSignalsFromFd()) {
          LOG(INFO) << "ServerManager: stop requested by signal (signalfd)";
        }
        if (stop_requested_) {
          return EXIT_SUCCESS;
        }
        continue;
      }

      std::map<int, Server>::iterator s_it = servers_.find(fd);
      if (s_it != servers_.end()) {
        LOG(DEBUG)
            << "Event is on server listen socket, accepting connections...";
        acceptConnection(fd);
        continue;
      }

      // Check if this is a CGI pipe FD
      std::map<int, int>::iterator cgi_it = cgi_pipe_to_conn_.find(fd);
      if (cgi_it != cgi_pipe_to_conn_.end()) {
        LOG(DEBUG) << "EPOLLIN event on CGI pipe fd: " << fd;
        handleCgiPipeEvent(fd);
        continue;
      }

      std::map<int, Connection>::iterator c_it = connections_.find(fd);
      if (c_it == connections_.end()) {
        LOG(DEBUG) << "Unknown fd: " << fd << ", skipping";
        continue; /* unknown fd */
      }

      Connection& c = c_it->second;
      uint32_t ev_mask = events[i].events;

      /* readable */
      if (ev_mask & EPOLLIN) {
        LOG(DEBUG) << "EPOLLIN event on connection fd: " << fd;
        int status = c.handleRead();

        if (status < 0) {
          LOG(DEBUG) << "handleRead failed, closing connection fd: " << fd;
          cleanupHandlerResources(c);
          close(fd);
          connections_.erase(fd);
          continue;
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
          LOG(DEBUG)
              << "handleWrite complete or failed, closing connection fd: "
              << fd;
          cleanupHandlerResources(c);
          close(fd);
          connections_.erase(fd);
        }
      }
    }

    /* After processing events, iterate connections to prepare responses
       for those that completed reading but don't yet have a write buffer. */
    LOG(DEBUG) << "Checking " << connections_.size()
               << " connection(s) for response preparation";
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

      LOG(DEBUG) << "Preparing response for connection fd: " << conn_fd;

      if (!conn.request.parseStartAndHeaders(conn.read_buffer,
                                             conn.headers_end_pos)) {
        /* malformed start line or headers -> 400 Bad Request */
        LOG(INFO) << "Malformed request on fd " << conn_fd
                  << ", sending 400 Bad Request";
        conn.prepareErrorResponse(http::S_400_BAD_REQUEST);
        updateEvents(conn_fd, EPOLLOUT | EPOLLET);
        continue;
      }

      // Extract body from read_buffer (after "\r\n\r\n")
      std::size_t body_start = conn.headers_end_pos + 4;
      // Check for Content-Length header
      std::string content_length_str = conn.request.getHeader("Content-Length");
      std::size_t expected_body_length = 0;
      bool has_content_length = false;
      if (!content_length_str.empty()) {
        try {
          expected_body_length = static_cast<std::size_t>(std::stoul(content_length_str));
          has_content_length = true;
        } catch (const std::exception& e) {
          // Malformed Content-Length header
          LOG(INFO) << "Malformed Content-Length header on fd " << conn_fd
                    << ", sending 400 Bad Request";
          conn.prepareErrorResponse(http::S_400_BAD_REQUEST);
          updateEvents(conn_fd, EPOLLOUT | EPOLLET);
          continue;
        }
      }
      std::size_t available_body_length = (body_start < conn.read_buffer.size())
          ? (conn.read_buffer.size() - body_start)
          : 0;
      if (has_content_length) {
        if (available_body_length < expected_body_length) {
          // Body not fully received yet, wait for more data
          continue;
        } else if (available_body_length > expected_body_length) {
          // More data than expected, only use expected length
          std::string body_data = conn.read_buffer.substr(body_start, expected_body_length);
          conn.request.getBody().data = body_data;
        } else {
          // Exact match
          std::string body_data = conn.read_buffer.substr(body_start, expected_body_length);
          conn.request.getBody().data = body_data;
        }
      } else {
        // No Content-Length header, use all available data
        if (available_body_length > 0) {
          std::string body_data = conn.read_buffer.substr(body_start);
          conn.request.getBody().data = body_data;
        }
      }

      LOG(DEBUG) << "Request parsed: " << conn.request.request_line.method
                 << " " << conn.request.request_line.uri;

      /* find the server that accepted this connection */
      std::map<int, Server>::iterator srv_it = servers_.find(conn.server_fd);
      if (srv_it == servers_.end()) {
        /* shouldn't happen, but handle gracefully */
        LOG(ERROR) << "Server not found for connection fd " << conn_fd
                   << " (server_fd: " << conn.server_fd << ")";
        conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
        updateEvents(conn_fd, EPOLLOUT | EPOLLET);
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
          registerCgiPipe(monitor_fd, conn_fd);
          // Don't enable EPOLLOUT yet - wait for CGI to complete
          continue;
        }
      }

      /* enable EPOLLOUT now that we have data to send */
      updateEvents(conn_fd, EPOLLOUT | EPOLLET);
    }
  }
  LOG(DEBUG) << "ServerManager: exiting event loop";
  return EXIT_SUCCESS;
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

  LOG(INFO) << "signals: signalfd installed and signals blocked";
}

bool ServerManager::processSignalsFromFd() {
  struct signalfd_siginfo fdsi;
  while (true) {
    ssize_t s = read(sfd_, &fdsi, sizeof(fdsi));
    if (s < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return stop_requested_;
      }
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
    LOG(INFO) << "signals: got unexpected signo=" << fdsi.ssi_signo;
  }
}

void ServerManager::shutdown() {
  LOG(INFO) << "Shutting down ServerManager...";

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

  LOG(INFO) << "ServerManager shutdown complete";
}

void ServerManager::registerCgiPipe(int pipe_fd, int conn_fd) {
  cgi_pipe_to_conn_[pipe_fd] = conn_fd;

  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = pipe_fd;

  if (epoll_ctl(efd_, EPOLL_CTL_ADD, pipe_fd, &ev) < 0) {
    LOG_PERROR(ERROR, "epoll_ctl ADD CGI pipe");
    cgi_pipe_to_conn_.erase(pipe_fd);
  }
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
  updateEvents(conn_fd, EPOLLOUT | EPOLLET);
}

void ServerManager::cleanupHandlerResources(Connection& c) {
  if (c.active_handler != NULL) {
    int monitor_fd = c.active_handler->getMonitorFd();
    if (monitor_fd >= 0) {
      unregisterCgiPipe(monitor_fd);
    }
  }
}
