#include "ServerManager.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "Logger.hpp"
#include "constants.hpp"
#include "utils.hpp"

ServerManager::ServerManager() : efd_(-1) {}

ServerManager::ServerManager(const ServerManager& other) : efd_(-1) {
  (void)other;
}

ServerManager& ServerManager::operator=(const ServerManager& other) {
  (void)other;
  return *this;
}

ServerManager::~ServerManager() {
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
      }
    } else {
      LOG_PERROR(ERROR, "epoll_ctl MOD");
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

  /* event loop */
  struct epoll_event events[MAX_EVENTS];
  LOG(INFO) << "Entering main event loop (waiting for connections)...";

  while (1) {
    int n = epoll_wait(efd_, events, MAX_EVENTS, -1);
    if (n < 0) {
      if (errno == EINTR) {
        LOG(DEBUG) << "epoll_wait interrupted by signal, continuing...";
        continue; /* interrupted by signal */
      }
      LOG_PERROR(ERROR, "epoll_wait");
      return EXIT_FAILURE;
    }

    LOG(DEBUG) << "epoll_wait returned " << n << " event(s)";

    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;
      LOG(DEBUG) << "Processing event for fd: " << fd;

      std::map<int, Server>::iterator s_it = servers_.find(fd);
      if (s_it != servers_.end()) {
        LOG(DEBUG)
            << "Event is on server listen socket, accepting connections...";
        acceptConnection(fd);
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
          close(fd);
          connections_.erase(fd);
          continue;
        }

        if (c.read_done) {
          LOG(DEBUG) << "Read complete on fd: " << fd << ", enabling EPOLLOUT";
          /* enable EPOLLOUT now that we have data to send */
          updateEvents(fd, EPOLLOUT | EPOLLET);
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

      if (!conn.read_done) {
        continue;
      }

      if (!conn.write_buffer.empty()) {
        continue; /* already prepared */
      }

      LOG(DEBUG) << "Preparing response for connection fd: " << conn_fd;

      /* find header/body separator */
      std::size_t headers_pos = conn.read_buffer.find(CRLF CRLF);
      if (headers_pos == std::string::npos) {
        LOG(DEBUG) << "Headers not complete yet for fd: " << conn_fd;
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
        LOG(INFO) << "Malformed request on fd " << conn_fd
                  << ", sending 400 Bad Request";
        conn.response.status_line.version = HTTP_VERSION;
        conn.response.status_line.status_code = http::S_400_BAD_REQUEST;
        conn.response.status_line.reason =
            http::reasonPhrase(http::S_400_BAD_REQUEST);
        conn.response.getBody().data =
            http::statusWithReason(conn.response.status_line.status_code);
        std::ostringstream oss;
        oss << conn.response.getBody().size();
        conn.response.addHeader("Content-Length", oss.str());
        conn.response.addHeader("Content-Type", "text/plain; charset=utf-8");
        conn.write_buffer = conn.response.serialize();
        updateEvents(conn_fd, EPOLLOUT | EPOLLET);
        continue;
      }
      LOG(DEBUG) << "Request parsed: " << conn.request.request_line.method
                 << " " << conn.request.request_line.uri;

      /* set body to remaining bytes after header separator */
      std::size_t body_start = headers_pos + 4; /* \r\n\r\n */
      conn.request.getBody().data.clear();
      if (conn.read_buffer.size() > body_start) {
        conn.request.getBody().data = conn.read_buffer.substr(body_start);
      }

      /* find the server that accepted this connection */
      std::map<int, Server>::iterator srv_it = servers_.find(conn.server_fd);
      if (srv_it == servers_.end()) {
        /* shouldn't happen, but handle gracefully */
        LOG(ERROR) << "Server not found for connection fd " << conn_fd
                   << " (server_fd: " << conn.server_fd << ")";
        conn.response.status_line.version = HTTP_VERSION;
        conn.response.status_line.status_code =
            http::S_500_INTERNAL_SERVER_ERROR;
        conn.response.status_line.reason =
            http::reasonPhrase(http::S_500_INTERNAL_SERVER_ERROR);
        conn.response.getBody().data =
            http::statusWithReason(conn.response.status_line.status_code);
        std::ostringstream oss_err;
        oss_err << conn.response.getBody().size();
        conn.response.addHeader("Content-Length", oss_err.str());
        conn.response.addHeader("Content-Type", "text/plain; charset=utf-8");
        conn.write_buffer = conn.response.serialize();
        updateEvents(conn_fd, EPOLLOUT | EPOLLET);
        continue;
      }

      LOG(DEBUG) << "Found server configuration for fd " << conn_fd
                 << " (port: " << srv_it->second.port << ")";

      /* prepare 200 OK response echoing the request */
      LOG(DEBUG) << "Preparing echo response for fd " << conn_fd;
      conn.response.status_line.version = HTTP_VERSION;
      conn.response.status_line.status_code = http::S_200_OK;
      conn.response.status_line.reason = http::reasonPhrase(http::S_200_OK);
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

  shutdown();
  return EXIT_SUCCESS;
}

void ServerManager::shutdown() {
  LOG(INFO) << "Shutting down ServerManager...";

  if (efd_ >= 0) {
    LOG(DEBUG) << "Closing epoll fd: " << efd_;
    close(efd_);
    efd_ = -1;
  }

  // close all connection fds
  LOG(DEBUG) << "Closing " << connections_.size() << " connection(s)";
  for (std::map<int, Connection>::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    close(it->first);
  }
  connections_.clear();

  // close listening fds
  LOG(DEBUG) << "Closing " << servers_.size() << " server socket(s)";
  for (std::map<int, Server>::iterator it = servers_.begin();
       it != servers_.end(); ++it) {
    it->second.disconnect();
  }
  servers_.clear();

  LOG(INFO) << "ServerManager shutdown complete";
}
