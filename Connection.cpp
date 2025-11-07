#include "Connection.hpp"
#include "Logger.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/socket.h>

Connection::Connection()
    : fd(-1), server_fd(-1), write_offset(0), read_done(false),
      write_ready(false), request(), response() {}

Connection::Connection(int fd)
    : fd(fd), server_fd(-1), write_offset(0), read_done(false),
      write_ready(false), request(), response() {}

Connection::Connection(const Connection &other)
    : fd(other.fd), server_fd(other.server_fd), read_buffer(other.read_buffer),
      write_buffer(other.write_buffer), write_offset(other.write_offset),
      read_done(other.read_done), write_ready(other.write_ready),
      request(other.request), response(other.response) {}

Connection::~Connection() {}

Connection &Connection::operator=(const Connection &other) {
  if (this != &other) {
    fd = other.fd;
    server_fd = other.server_fd;
    read_buffer = other.read_buffer;
    write_buffer = other.write_buffer;
    write_offset = other.write_offset;
    read_done = other.read_done;
    write_ready = other.write_ready;
    request = other.request;
    response = other.response;
  }
  return *this;
}

int Connection::handleRead() {
  while (1) {
    char buf[WRITE_BUF_SIZE] = {0};

    ssize_t r = recv(fd, buf, sizeof(buf), 0);

    if (r < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return 1;
      error("read");
      return -1;
    }

    if (r == 0) {
      LOG(INFO) << "=== Client disconnected ===";
      LOG(INFO) << "File descriptor: " << fd;
      return -1;
    }

    // Add new data to persistent buffer
    read_buffer.append(buf, r);

    // Check if the HTTP request is complete
    if (read_buffer.find(CRLF CRLF) != std::string::npos) {
      read_done = true;
      break;
    }
  }
  return 0;
}

int Connection::handleWrite() {
  while (write_offset < write_buffer.size()) {
    ssize_t w =
        send(fd, write_buffer.c_str() + write_offset,
             static_cast<size_t>(write_buffer.size()) - write_offset, 0);

    LOG(DEBUG) << "Sent " << w << " bytes to fd=" << fd;

    if (w < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return 1;
      // Error occurred
      error("write");
      return -1;
    }

    write_offset += static_cast<size_t>(w);
  }

  // All data sent successfully
  return 0;
}
