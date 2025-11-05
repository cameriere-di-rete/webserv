#include "ServerManager.hpp"
#include "Connection.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

ServerManager::ServerManager() : _efd(-1) {}

ServerManager::ServerManager(const ServerManager &other) : _efd(-1) {
  (void)other;
}

ServerManager &ServerManager::operator=(const ServerManager &other) {
  (void)other;
  return *this;
}

ServerManager::~ServerManager() {
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
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      error("accept");
      break;
    }
    if (set_nonblocking(conn_fd) < 0) {
      error("set_nonblocking conn_fd");
      close(conn_fd);
      continue;
    }

    std::cout << "=== New connection accepted ===" << std::endl;
    std::cout << "File descriptor: " << conn_fd << std::endl;

    Connection connection(conn_fd);
    _connections[conn_fd] = connection;

    // watch for reads; no write interest yet
    updateEvents(conn_fd, EPOLLIN | EPOLLET);
  }
}

void ServerManager::updateEvents(int fd, uint32_t events) {
  if (_efd < 0) {
    std::cerr << "epoll fd not initialized\n";
    return;
  }

  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(_efd, EPOLL_CTL_MOD, fd, &ev) < 0) {
    if (errno == ENOENT) {
      if (epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        error("epoll_ctl ADD");
      }
    } else {
      error("epoll_ctl MOD");
    }
  }
}

int ServerManager::run() {
  /* create epoll instance */
  _efd = epoll_create1(0);
  if (_efd < 0) {
    return error("epoll_create1");
  }

  /* register listener fds */
  for (std::map<int, Server>::const_iterator it = _servers.begin();
       it != _servers.end(); ++it) {
    int listen_fd = it->first;
    struct epoll_event ev;
    ev.events = EPOLLIN; /* only need read events for the listener */
    ev.data.fd = listen_fd;
    if (epoll_ctl(_efd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
      return error("epoll_ctl ADD listen_fd");
    }
  }

  /* event loop */
  struct epoll_event events[MAX_EVENTS];

  while (1) {
    int n = epoll_wait(_efd, events, MAX_EVENTS, -1);
    if (n < 0) {
      if (errno == EINTR)
        continue; /* interrupted by signal */
      return error("epoll_wait");
    }

    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;

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

      if (!conn.read_done)
        continue;

      if (!conn.write_buffer.empty())
        continue; /* already prepared */

      /* find header/body separator */
      std::size_t headers_pos = conn.read_buffer.find(CRLF CRLF);
      if (headers_pos == std::string::npos)
        continue; /* wait for headers */

      /* split header part into lines */
      std::vector<std::string> lines;
      std::string temp;
      for (std::size_t i = 0; i < headers_pos; ++i) {
        char ch = conn.read_buffer[i];
        if (ch == '\r')
          continue;
        if (ch == '\n') {
          lines.push_back(temp);
          temp.clear();
        } else {
          temp.push_back(ch);
        }
      }

      if (!temp.empty())
        lines.push_back(temp);

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
      if (conn.read_buffer.size() > body_start)
        conn.request.getBody().data = conn.read_buffer.substr(body_start);

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

  shutdown();
  return EXIT_SUCCESS;
}

void ServerManager::shutdown() {
  if (_efd >= 0) {
    close(_efd);
    _efd = -1;
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
