#include "ServerManager.hpp"
#include "Connection.hpp"
#include "Logger.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

ServerManager::ServerManager() : _efd(-1), _sfd(-1), _stop_requested(false) {}

ServerManager::ServerManager(const ServerManager &other)
    : _efd(-1), _sfd(-1), _stop_requested(false) {
  (void)other;
}

ServerManager &ServerManager::operator=(const ServerManager &other) {
  (void)other;
  return *this;
}

ServerManager::~ServerManager() {
  LOG(DEBUG) << "Shutting down ServerManager...";
  shutdown();
}

void ServerManager::initServers(const std::vector<int> &ports) {
  for (std::vector<int>::const_iterator it = ports.begin(); it != ports.end();
       ++it) {
    Server server(*it);
    server.init();
    /* store by listening fd */
    _servers[server.fd] = server;
    /* prevent server destructor from closing the fd of the temporary */
    server.fd = -1;
  }
}

void ServerManager::acceptConnection(int listen_fd) {
  while (1) {
    int conn_fd = accept(listen_fd, NULL, NULL);
    if (conn_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
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

    LOG(INFO) << "New connection accepted (fd: " << conn_fd << ")";

    Connection connection(conn_fd);
    /* record which listening/server fd accepted this connection */
    connection.server_fd = listen_fd;
    _connections[conn_fd] = connection;

    // watch for reads; no write interest yet
    updateEvents(conn_fd, EPOLLIN | EPOLLET);
  }
}

void ServerManager::updateEvents(int fd, uint32_t events) {
  if (_efd < 0) {
    LOG(ERROR) << "epoll fd not initialized";
    return;
  }

  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(_efd, EPOLL_CTL_MOD, fd, &ev) < 0) {
    if (errno == ENOENT) {
      if (epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        LOG_PERROR(ERROR, "epoll_ctl ADD");
      }
    } else {
      LOG_PERROR(ERROR, "epoll_ctl MOD");
    }
  }
}

int ServerManager::run() {
  /* create epoll instance */
  _efd = epoll_create1(0);
  if (_efd < 0) {
    LOG_PERROR(ERROR, "epoll_create1");
    return EXIT_FAILURE;
  }

  /* register listener fds */
  for (std::map<int, Server>::const_iterator it = _servers.begin();
       it != _servers.end(); ++it) {
    int listen_fd = it->first;
    struct epoll_event ev;
    ev.events = EPOLLIN; /* only need read events for the listener */
    ev.data.fd = listen_fd;
    if (epoll_ctl(_efd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
      LOG_PERROR(ERROR, "epoll_ctl ADD listen_fd");
      return EXIT_FAILURE;
    }
  }

  /* register signalfd (if available) so signals are delivered as FD events */
  if (_sfd >= 0) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = _sfd;
    if (epoll_ctl(_efd, EPOLL_CTL_ADD, _sfd, &ev) < 0) {
      LOG_PERROR(ERROR, "epoll_ctl ADD signalfd");
      // non-fatal: continue without signalfd
    }
  }

  /* event loop */
  struct epoll_event events[MAX_EVENTS];

  while (!_stop_requested) {
    int n = epoll_wait(_efd, events, MAX_EVENTS, -1);
    if (n < 0) {
      if (errno == EINTR) {
        if (_stop_requested) {
          LOG(INFO)
              << "ServerManager: stop requested by signal, exiting event loop";
          break;
        }
        continue; /*r::_stop_requested const { interrupted by non-termination
                     signal */
      }
      LOG_PERROR(ERROR, "epoll_wait");
      break;
    }

    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;

      if (_sfd >= 0 && fd == _sfd) {
        // process pending signals from signalfd
        if (processSignalsFromFd()) {
          LOG(INFO) << "ServerManager: stop requested by signal (signalfd)";
        }
        if (_stop_requested) {
          break; // break out of for-loop; outer while will exit after check
        }
        continue;
      }

      std::map<int, Server>::iterator s_it = _servers.find(fd);
      if (s_it != _servers.end()) {
        acceptConnection(fd);
        continue;
      }

      std::map<int, Connection>::iterator c_it = _connections.find(fd);
      if (c_it == _connections.end()) {
        continue; /* unknown fd */
      }

      Connection &c = c_it->second;
      uint32_t ev_mask = events[i].events;

      /* readable */
      if (ev_mask & EPOLLIN) {
        int status = c.handleRead();

        if (status < 0) {
          close(fd);
          _connections.erase(fd);
          continue;
        }

        if (c.read_done) {
          /* enable EPOLLOUT now that we have data to send */
          updateEvents(fd, EPOLLOUT | EPOLLET);
        }
      }

      /* writable */
      if (ev_mask & EPOLLOUT) {
        int status = c.handleWrite();

        if (status <= 0) {
          close(fd);
          _connections.erase(fd);
        }
      }
    }

    /* After processing events, iterate connections to prepare responses
       for those that completed reading but don't yet have a write buffer. */
    for (std::map<int, Connection>::iterator it = _connections.begin();
         it != _connections.end(); ++it) {
      Connection &conn = it->second;
      int conn_fd = it->first;

      if (!conn.read_done) {
        continue;
      }

      if (!conn.write_buffer.empty()) {
        continue; /* already prepared */
      }

      /* find header/body separator */
      std::size_t headers_pos = conn.read_buffer.find(CRLF CRLF);
      if (headers_pos == std::string::npos) {
        continue; /* wait for headers */
      }

      /* split header part into lines */
      std::vector<std::string> lines;
      std::string temp;
      for (std::size_t i = 0; i < headers_pos; ++i) {
        char ch = conn.read_buffer[i];
        if (ch == '\r') {
          continue;
        }
        if (ch == '\n') {
          lines.push_back(temp);
          temp.clear();
        } else {
          temp.push_back(ch);
        }
      }

      if (!temp.empty()) {
        lines.push_back(temp);
      }

      if (!conn.request.parseStartAndHeaders(lines)) {
        /* malformed start line or headers -> 400 Bad Request */
        conn.response.status_line.version = HTTP_VERSION;
        conn.response.status_line.status_code = 400;
        conn.response.status_line.reason = "Bad Request";
        conn.response.getBody().data = "400 Bad Request";
        std::ostringstream oss;
        oss << conn.response.getBody().size();
        conn.response.addHeader("Content-Length", oss.str());
        conn.response.addHeader("Content-Type", "text/plain; charset=utf-8");
        conn.write_buffer = conn.response.serialize();
        updateEvents(conn_fd, EPOLLOUT | EPOLLET);
        continue;
      }

      /* set body to remaining bytes after header separator */
      std::size_t body_start = headers_pos + 4; /* \r\n\r\n */
      conn.request.getBody().data.clear();
      if (conn.read_buffer.size() > body_start) {
        conn.request.getBody().data = conn.read_buffer.substr(body_start);
      }

      /* prepare 200 OK response echoing the request body */
      conn.response.status_line.version = HTTP_VERSION;
      conn.response.status_line.status_code = 200;
      conn.response.status_line.reason = "OK";
      conn.response.setBody(Body(conn.read_buffer));
      std::ostringstream oss2;
      oss2 << conn.response.getBody().size();
      conn.response.addHeader("Content-Length", oss2.str());
      conn.response.addHeader("Content-Type", "text/plain; charset=utf-8");
      conn.write_buffer = conn.response.serialize();

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
  sigaddset(&mask, SIGHUP);

  if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
    LOG_PERROR(ERROR, "sigprocmask");
    return;
  }

  // Create signalfd
  _sfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
  if (_sfd < 0) {
    LOG_PERROR(ERROR, "signalfd");
    return;
  }

  // Ignore SIGPIPE
  struct sigaction sa_pipe;
  std::memset(&sa_pipe, 0, sizeof(sa_pipe));
  sa_pipe.sa_handler = SIG_IGN;
  sigemptyset(&sa_pipe.sa_mask);
  if (sigaction(SIGPIPE, &sa_pipe, NULL) < 0) {
    LOG_PERROR(ERROR, "sigaction(SIGPIPE)");
  }

  LOG(INFO) << "signals: signalfd installed and signals blocked";
}

bool ServerManager::processSignalsFromFd() {
  if (_sfd < 0) {
    return _stop_requested;
  }

  struct signalfd_siginfo fdsi;
  while (1) {
    ssize_t s = read(_sfd, &fdsi, sizeof(fdsi));
    if (s < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return _stop_requested;
      }
      LOG_PERROR(ERROR, "read(signalfd)");
      return _stop_requested;
    }
    if (s != sizeof(fdsi)) {
      continue;
    }

    // Handle the signal
    if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM) {
      _stop_requested = true;
      return true;
    }
    if (fdsi.ssi_signo == SIGHUP) {
      LOG(INFO) << "signals: SIGHUP received";
      continue;
    }
    LOG(INFO) << "signals: got signo=" << fdsi.ssi_signo;
  }
}

void ServerManager::shutdown() {
  if (_efd >= 0) {
    close(_efd);
    _efd = -1;
  }
  if (_sfd >= 0) {
    close(_sfd);
    _sfd = -1;
  }
  // close all connection fds
  for (std::map<int, Connection>::iterator it = _connections.begin();
       it != _connections.end(); ++it) {
    close(it->first);
  }
  _connections.clear();

  // close listening fds
  for (std::map<int, Server>::iterator it = _servers.begin();
       it != _servers.end(); ++it) {
    it->second.disconnect();
  }
  _servers.clear();
}
