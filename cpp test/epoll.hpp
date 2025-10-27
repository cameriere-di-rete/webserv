#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <cstddef>
#include <cstring>

#define WRITE_BUF_SIZE 4096

class conn_state_t {
private:
    int fd;
    char write_buf[WRITE_BUF_SIZE];
    size_t write_len;
    size_t write_off;

public:
    conn_state_t(int fd) : fd(fd), write_len(0), write_off(0) {
        std::memset(write_buf, 0, WRITE_BUF_SIZE);
    }

    int get_fd() const { return fd; }
    char* get_write_buf() { return write_buf; }
    size_t get_write_len() const { return write_len; }
    size_t get_write_off() const { return write_off; }

    void set_write_len(size_t len) { write_len = len; }
    void set_write_off(size_t off) { write_off = off; }
};

void die(const char *msg);
int set_nonblocking(int fd);
void update_events(int efd, int fd, unsigned int events);

#endif