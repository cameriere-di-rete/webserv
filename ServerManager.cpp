#include "ServerManager.hpp"
#include "constants.hpp"
#include "utils.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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
    if (conn_fd == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      error("accept");
      break;
    }
    if (set_nonblocking(conn_fd) == -1) {
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
    printError("epoll fd not initialized");
    return;
  }

  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(_efd, EPOLL_CTL_MOD, fd, &ev) == -1) {
    if (errno == ENOENT) {
      if (epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &ev) == -1) {
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
  if (_efd == -1) {
    return error("epoll_create1");
  }

  /* register listener fds */
  for (std::map<int, Server>::const_iterator it = _servers.begin();
       it != _servers.end(); ++it) {
    int listen_fd = it->first;
    struct epoll_event ev;
    ev.events = EPOLLIN; /* only need read events for the listener */
    ev.data.fd = listen_fd;
    if (epoll_ctl(_efd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
      return error("epoll_ctl ADD listen_fd");
    }
  }

  /* event loop */
  struct epoll_event events[MAX_EVENTS];

  while (1) {
    int n = epoll_wait(_efd, events, MAX_EVENTS, -1);
    if (n == -1) {
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
        while (1) {
          char buf[WRITE_BUF_SIZE] = {0};
          ssize_t r = recv(fd, buf, sizeof(buf), 0);
          if (r == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
              break;
            error("read");
            close(fd);
            _connections.erase(fd);
            break;
          }
          if (r == 0) { /* client closed */
            std::cout << "=== Client disconnected ===" << std::endl;
            std::cout << "File descriptor: " << fd << std::endl;
            close(fd);
            _connections.erase(fd);
            break;
          }

          std::cout << "=== HTTP request received ===" << std::endl;
          std::cout << "File descriptor: " << fd << std::endl;
          std::cout << "Bytes received: " << r << std::endl;
          std::cout << "Content:" << std::endl;
          std::cout << std::string(buf, r) << std::endl;
          std::cout << "===========================" << std::endl;

          c.write_buffer = "HTTP/1.0 200 OK" CRLF "Content-Type: text/plain; "
                           "charset=utf-8" CRLF CRLF;

          c.write_buffer.append(buf, static_cast<size_t>(r));

          /* enable EPOLLOUT now that we have data to send */
          updateEvents(fd, EPOLLIN | EPOLLOUT | EPOLLET);
        }
      }

      /* writable */
      if (ev_mask & EPOLLOUT) {
        while (c.write_offset < c.write_buffer.size()) {
          ssize_t w = send(
              fd, c.write_buffer.c_str() + c.write_offset,
              static_cast<size_t>(c.write_buffer.size()) - c.write_offset, 0);

          printf("Sent %zd bytes to fd=%d\n", w, fd);

          if (w == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
              break;
            error("write");
            close(fd);
            _connections.erase(fd);
            break;
          }
          c.write_offset += static_cast<size_t>(w);
        }

        /* If everything was sent, stop watching EPOLLOUT */
        if (c.write_offset == c.write_buffer.size()) {
          c.write_buffer.clear();
          c.write_offset = 0;
          updateEvents(fd, EPOLLIN | EPOLLET); /* drop EPOLLOUT */
        }
      }
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
