#include "Server.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

int main(void) {
  // handle and validate config file

  std::vector<Server> servers;

  // loop and instanciate Servers
  try {
    Server server(8080);
    std::cout << "Listening on port " << server.port << "..." << std::endl;
    servers.push_back(server);
    server.fd = -1;
  } catch (char *s) {
    std::cerr << "Error while initiating Server: " << s << std::endl;
    return 1;
  }

  /* ignore SIGPIPE so a broken client won't kill the process when we write */
  signal(SIGPIPE, SIG_IGN);

  for (;;) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd =
        accept(servers[0].fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    std::cout << "Accepted connection from " << inet_ntoa(client_addr.sin_addr)
              << ":" << ntohs(client_addr.sin_port) << std::endl;

    char buffer[4096];
    ssize_t n;
    while ((n = read(client_fd, buffer, sizeof(buffer))) > 0) {
      std::cout.write(buffer, n);
      std::cout.flush();
    }
    if (n < 0) {
      perror("read");
    }

    close(client_fd);
    std::cout << std::endl << "Connection closed" << std::endl;
  }

  close(servers[0].fd);
  return 0;
}
