#include "Connection.hpp"
#include "Server.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

int error(const char *s) {
  std::cerr << s << ": " << strerror(errno) << std::endl;
  return EXIT_FAILURE;
}

int main(void) {
  // handle and validate config file

  std::map<int, Server> servers;

  // loop and instanciate Servers
  try {
    Server server(8080);
    server.init();
    servers[0] = server;
    server.fd = -1;
  } catch (char *s) {
    std::cerr << "Error while initiating Server: " << s << std::endl;
    return 1;
  }

  /* ignore SIGPIPE so a broken client won't kill the process when we write */
  signal(SIGPIPE, SIG_IGN);

  /* ----- create epoll instance ----- */
  int efd = epoll_create1(0);
  if (efd == -1) {
    return error("epoll_create1");
  }

  struct epoll_event ev;
  ev.events = EPOLLIN; /* only need read events for the listener */
  ev.data.fd = servers[0].fd;
  if (epoll_ctl(efd, EPOLL_CTL_ADD, servers[0].fd, &ev) == -1) {
    return error("epoll_ctl ADD listen_fd");
  }

  /* ----- event loop ----- */
  std::map<int, Connection> connections;
  struct epoll_event events[MAX_EVENTS];

  while (1) {
    int n = epoll_wait(efd, events, MAX_EVENTS, -1);
    if (n == -1) {
      if (errno == EINTR)
      continue; /* interrupted by signal */
      return error("epoll_wait");
    }
    
    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;
      
      /* ----- new incoming connection ----- */
      std::cout << "Checking fd=" << fd << std::endl;
      std::map<int, Server>::iterator s = servers.find(0);
      if (s != servers.end()) {
        while (1) {
          int conn_fd = accept(s->second.fd, NULL, NULL);
          if (conn_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
            perror("accept");
            break;
          }
          if (set_nonblocking(conn_fd) == -1) {
            perror("set_nonblocking conn_fd");
            close(conn_fd);
            continue;
          }

          std::cout << "=== Nuova connessione accettata ===" << std::endl;
          std::cout << "File descriptor: " << conn_fd << std::endl;

          /* allocate per‑connection state */
          Connection connection(conn_fd);
          connections[conn_fd] = connection;

          /* watch for reads; no write interest yet */
          update_events(efd, conn_fd, EPOLLIN | EPOLLET);
        }
        continue;
      }

      if (connections.find(fd) == connections.end()) {
        continue; /* should not happen */
      }

      /* ----- existing client ----- */
      Connection &c = connections[fd];

      uint32_t ev_mask = events[i].events;

      /* ----- readable ----- */
      if (ev_mask & EPOLLIN) {
        while (1) {
          char buf[WRITE_BUF_SIZE];
          ssize_t r = read(fd, buf, sizeof(buf));
          if (r == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
              break;
            perror("read");
            close(fd);
            connections.erase(fd);
            break;
          }
          if (r == 0) { /* client closed */
            std::cout << "=== Client disconnesso ===" << std::endl;
            std::cout << "File descriptor: " << fd << std::endl;
            close(fd);
            connections.erase(fd);
            break;
          }

          std::cout << "=== Richiesta HTTP ricevuta ===" << std::endl;
          std::cout << "File descriptor: " << fd << std::endl;
          std::cout << "Bytes ricevuti: " << r << std::endl;
          std::cout << "Contenuto:" << std::endl;
          std::cout << std::string(buf, r) << std::endl;
          std::cout << "===========================" << std::endl;

          /* copy data into the connection's write buffer */
          c.in.append(buf);

          /* enable EPOLLOUT now that we have data to send */
          update_events(efd, fd, EPOLLIN | EPOLLOUT | EPOLLET);
        }
      }

      /* ----- writable ----- */
      if (ev_mask & EPOLLOUT) {
        while (c.write_offset < c.in.size()) {
          ssize_t w = write(fd, "messaggio ricevuto\n", 18);

          printf("Inviati %zd bytes a fd=%d\n", w, fd);

          if (w == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
              break;
            perror("write");
            close(fd);
            connections.erase(fd);
            break;
          }
          c.write_offset += (size_t)w;
        }

        /* If everything was sent, stop watching EPOLLOUT */
        if (c.write_offset == c.in.size()) {
          c.in.clear();
          c.write_offset = 0;
          update_events(efd, fd, EPOLLIN | EPOLLET); /* drop EPOLLOUT */
        }
      }
    }
  }

  /* Cleanup (unreachable in this infinite loop, but good practice) */
  close(efd);
  return 0;
}
