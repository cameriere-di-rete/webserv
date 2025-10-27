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

// Global array of connection states (definition)
conn_state_t* states[MAX_FDS] = {0};

conn_state_t::conn_state_t(int fd) : fd(fd), write_len(0), write_off(0) 
{
    std::memset(write_buf, 0, WRITE_BUF_SIZE);
}


    // Getters
    int conn_state_t::get_fd() const
    {
        return fd;
    }
    char* conn_state_t::get_write_buf()
    {
        return write_buf;
    }
    size_t conn_state_t::get_write_len() const
    {
        return write_len;
    }
    size_t conn_state_t::get_write_off() const
    {
        return write_off;
    }

    // Setters
    void conn_state_t::set_write_len(size_t len)
    {
        write_len = len;
    }
    void conn_state_t::set_write_off(size_t off)
    {
        write_off = off;
    }
    
    // Utility methods
    void conn_state_t::add_write_off(size_t n)
    {
        write_off += n;
    }

    void conn_state_t::reset_write()
    { 
        write_len = 0; 
        write_off = 0; 
    }

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
            // fd not yet in the set -> add it
            if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) == -1)
                die("epoll_ctl ADD (update_events)");
        } else {
            die("epoll_ctl MOD");
        }
    }
}