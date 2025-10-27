#include "epoll.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
        die("socket");

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = 0;
    std::memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        die("bind");
    if (listen(listen_fd, SOMAXCONN) == -1) die("listen");
    if (set_nonblocking(listen_fd) == -1) die("set_nonblocking listen_fd");

    printf("Server in ascolto sulla porta %d\n", port);
    printf("In attesa di connessioni...\n");

    return 0;
}