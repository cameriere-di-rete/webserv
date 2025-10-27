#include "epoll.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_EVENTS      64
#define WRITE_BUF_SIZE 4096
#define MAX_FDS        10240

// Global array of connection states
conn_state_t* states[MAX_FDS] = {0};

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void update_events(int efd, int fd, unsigned int events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        if (errno == ENOENT) {
            if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) == -1)
                die("epoll_ctl ADD (update_events)");
        } else {
            die("epoll_ctl MOD");
        }
    }
}