#include "epoll.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <cstring>

#define MAX_EVENTS 64

int main(int argc, char *argv[]) {
if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(EXIT_FAILURE);
}
int port = atoi(argv[1]);

// Create listening socket
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
if (listen_fd == -1)
    die("socket");

int opt = 1;
setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

struct sockaddr_in addr;
std::memset(&addr, 0, sizeof(addr));
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = INADDR_ANY;
addr.sin_port = htons(port);

if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    die("bind");
if (listen(listen_fd, SOMAXCONN) == -1) 
    die("listen");
if (set_nonblocking(listen_fd) == -1) 
    die("set_nonblocking listen_fd");

printf("Server in ascolto sulla porta %d\n", port);
printf("In attesa di connessioni...\n");

// Create epoll instance
int efd = epoll_create1(0);
if (efd == -1) 
    die("epoll_create1");

struct epoll_event ev;
ev.events = EPOLLIN;
ev.data.fd = listen_fd;
if (epoll_ctl(efd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
    die("epoll_ctl ADD listen_fd");

// Event loop
struct epoll_event events[MAX_EVENTS];

while (1)
{
    int n = epoll_wait(efd, events, MAX_EVENTS, -1);
    if (n == -1)
    {
        if (errno == EINTR) continue;
        die("epoll_wait");
    }

    for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;

        // New incoming connection
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

                printf("Nuova connessione accettata: fd=%d\n", conn_fd);

                // Allocate per-connection state
                conn_state_t *cs = new conn_state_t(conn_fd);
                states[conn_fd] = cs;

                // Watch for reads; no write interest yet
                update_events(efd, conn_fd, EPOLLIN | EPOLLET);
            }
            continue;
        }

        // Existing client connection
        conn_state_t *c = states[fd];
        if (!c) continue;

        unsigned int ev_mask = events[i].events;

        // Readable event
        if (ev_mask & EPOLLIN) {
            while (1) {
                char buf[WRITE_BUF_SIZE];
                ssize_t r = read(fd, buf, sizeof(buf));
                if (r == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    perror("read");
                    close(fd);
                    delete c;
                    states[fd] = NULL;
                    break;
                }
                if (r == 0) {
                    // Client closed connection
                    printf("Client disconnesso: fd=%d\n", fd);
                    close(fd);
                    delete c;
                    states[fd] = NULL;
                    break;
                }

                printf("Ricevuti %ld bytes da fd=%d\n", (long)r, fd);

                // Copy data into the connection's write buffer
                size_t to_copy = (size_t)r;
                if (to_copy > WRITE_BUF_SIZE) 
                    to_copy = WRITE_BUF_SIZE;
                
                std::memcpy(c->get_write_buf(), buf, to_copy);
                c->set_write_len(to_copy);
                c->set_write_off(0);

                // Enable EPOLLOUT now that we have data to send
                update_events(efd, fd, EPOLLIN | EPOLLOUT | EPOLLET);
            }
        }

        // Writable event
        if (ev_mask & EPOLLOUT) {
            while (c->get_write_off() < c->get_write_len()) {
                ssize_t w = write(fd,
                                c->get_write_buf() + c->get_write_off(),
                                c->get_write_len() - c->get_write_off());
                if (w == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    perror("write");
                    close(fd);
                    delete c;
                    states[fd] = NULL;
                    break;
                }

                printf("Inviati %ld bytes a fd=%d\n", (long)w, fd);
                c->add_write_off((size_t)w);
            }

            // If everything was sent, stop watching EPOLLOUT
            if (c->get_write_off() == c->get_write_len()) {
                c->reset_write();
                update_events(efd, fd, EPOLLIN | EPOLLET);
            }
        }
    }
}

// Cleanup (unreachable in infinite loop, but good practice)
close(listen_fd);
close(efd);
return 0;
}