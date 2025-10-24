/* epoll_echo.c
 * Minimal echo server that demonstrates read‑ and write‑readiness
 * monitoring with epoll (EPOLLIN and EPOLLOUT).
 *
 * Compile: gcc -Wall -O2 -o epoll_echo epoll_echo.c
 * Run:     ./epoll_echo <port>
 *
 * The server:
 *   • Accepts new connections (non‑blocking).
 *   • Reads incoming data, stores it in a per‑connection write buffer.
 *   • Enables EPOLLOUT only while there is pending data to send.
 *   • Writes back the data (echo) when the socket becomes writable.
 *   • Uses edge‑triggered mode (EPOLLET) for efficiency.
 */

#define _GNU_SOURCE
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
#define MAX_FDS        10240   /* simple static mapping fd → state */

/* ---------- per‑connection state ---------- */
typedef struct {
    int   fd;                                 /* socket descriptor */
    char  write_buf[WRITE_BUF_SIZE];          /* pending data to send */
    size_t write_len;                         /* bytes currently in buffer */
    size_t write_off;                         /* how many bytes already written */
} conn_state_t;

/* ---------- globals ---------- */
static conn_state_t *states[MAX_FDS] = {0};

/* ---------- utility functions ---------- */
static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/* make a file descriptor non‑blocking */
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* (re)register interest list for a fd */
static void update_events(int efd, int fd, uint32_t events) {
    struct epoll_event ev = { .events = events, .data.fd = fd };
    if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        if (errno == ENOENT) {
            /* fd not yet in the set → add it */
            if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) == -1)
                die("epoll_ctl ADD (update_events)");
        } else {
            die("epoll_ctl MOD");
        }
    }
}

/* ---------- main ---------- */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);

    /* ----- create listening socket ----- */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) die("socket");

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        die("bind");
    if (listen(listen_fd, SOMAXCONN) == -1) die("listen");
    if (set_nonblocking(listen_fd) == -1) die("set_nonblocking listen_fd");

    /* ----- create epoll instance ----- */
    int efd = epoll_create1(0);
    if (efd == -1) die("epoll_create1");

    struct epoll_event ev = {0};
    ev.events = EPOLLIN;               /* only need read events for the listener */
    ev.data.fd = listen_fd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
        die("epoll_ctl ADD listen_fd");

    /* ----- event loop ----- */
    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int n = epoll_wait(efd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;   /* interrupted by signal */
            die("epoll_wait");
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            /* ----- new incoming connection ----- */
            if (fd == listen_fd) {
                while (1) {
                    int conn_fd = accept(listen_fd, NULL, NULL);
                    if (conn_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("accept");
                        break;
                    }
                    if (set_nonblocking(conn_fd) == -1) {
                        perror("set_nonblocking conn_fd");
                        close(conn_fd);
                        continue;
                    }

                    /* allocate per‑connection state */
                    conn_state_t *cs = calloc(1, sizeof(conn_state_t));
                    if (!cs) {
                        perror("calloc");
                        close(conn_fd);
                        continue;
                    }
                    cs->fd = conn_fd;
                    states[conn_fd] = cs;

                    /* watch for reads; no write interest yet */
                    update_events(efd, conn_fd, EPOLLIN | EPOLLET);
                }
                continue;
            }

            /* ----- existing client ----- */
            conn_state_t *c = states[fd];
            if (!c) continue;   /* should not happen */

            uint32_t ev_mask = events[i].events;

            /* ----- readable ----- */
            if (ev_mask & EPOLLIN) {
                while (1) {
                    char buf[WRITE_BUF_SIZE];
                    ssize_t r = read(fd, buf, sizeof(buf));
                    if (r == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("read");
                        close(fd);
                        free(c);
                        states[fd] = NULL;
                        break;
                    }
                    if (r == 0) {               /* client closed */
                        close(fd);
                        free(c);
                        states[fd] = NULL;
                        break;
                    }

                    /* copy data into the connection's write buffer */
                    size_t to_copy = (size_t)r;
                    if (to_copy > WRITE_BUF_SIZE) to_copy = WRITE_BUF_SIZE;
                    memcpy(c->write_buf, buf, to_copy);
                    c->write_len = to_copy;
                    c->write_off = 0;

                    /* enable EPOLLOUT now that we have data to send */
                    update_events(efd, fd, EPOLLIN | EPOLLOUT | EPOLLET);
                }
            }

            /* ----- writable ----- */
            if (ev_mask & EPOLLOUT) {
                while (c->write_off < c->write_len) {
                    ssize_t w = write(fd,
                                      c->write_buf + c->write_off,
                                      c->write_len - c->write_off);
                    if (w == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("write");
                        close(fd);
                        free(c);
                        states[fd] = NULL;
                        break;
                    }
                    c->write_off += (size_t)w;
                }

                /* If everything was sent, stop watching EPOLLOUT */
                if (c->write_off == c->write_len) {
                    c->write_len = c->write_off = 0;
                    update_events(efd, fd, EPOLLIN | EPOLLET);  /* drop EPOLLOUT */
                }
            }
        }
    }

    /* Cleanup (unreachable in this infinite loop, but good practice) */
    close(listen_fd);
    close(efd);
    return 0;
}
