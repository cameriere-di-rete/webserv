#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <cstddef>
#include <cstring>

#define WRITE_BUF_SIZE 4096
#define MAX_FDS 10240

// Per-connection state class
class conn_state_t
{
private:
    int fd;
    char write_buf[WRITE_BUF_SIZE];
    size_t write_len;
    size_t write_off;

public:
    conn_state_t(int fd);
    // Getters
    int get_fd() const;
    char* get_write_buf();
    size_t get_write_len() const;
    size_t get_write_off() const;
    // Setters
    void set_write_len(size_t len);
    void set_write_off(size_t off);
    // Utility methods
    void add_write_off(size_t n);
    void reset_write();
};

// Global array of connection states
extern conn_state_t* states[MAX_FDS];

// Utility functions
void die(const char *msg);
int set_nonblocking(int fd);
void update_events(int efd, int fd, unsigned int events);

#endif