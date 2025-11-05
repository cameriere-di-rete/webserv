#include "Connection.hpp"
#include "FileHandler.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

Connection::Connection()
    : fd(-1), write_offset(0), read_done(false), write_ready(false), request(),
      response(), file_fd(-1), file_offset(0), file_size(0),
      sending_file(false) {}

Connection::Connection(int fd)
    : fd(fd), write_offset(0), read_done(false), write_ready(false), request(),
      response(), file_fd(-1), file_offset(0), file_size(0),
      sending_file(false) {}

Connection::Connection(const Connection &other)
    : fd(other.fd), read_buffer(other.read_buffer),
      write_buffer(other.write_buffer), write_offset(other.write_offset),
      read_done(other.read_done), write_ready(other.write_ready),
      request(other.request), response(other.response), file_fd(other.file_fd),
      file_offset(other.file_offset), file_size(other.file_size),
      sending_file(other.sending_file) {}

Connection::~Connection() {}

Connection &Connection::operator=(const Connection &other) {
  if (this != &other) {
    fd = other.fd;
    read_buffer = other.read_buffer;
    read_done = other.read_done;
    write_buffer = other.write_buffer;
    write_offset = other.write_offset;
    write_ready = other.write_ready;
    request = other.request;
    response = other.response;
    file_fd = other.file_fd;
    file_offset = other.file_offset;
    file_size = other.file_size;
    sending_file = other.sending_file;
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
      std::cout << "=== Client disconnected ===" << std::endl;
      std::cout << "File descriptor: " << fd << std::endl;
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

    printf("Sent %zd bytes to fd=%d\n", w, fd);

    if (w < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return 1;
      // Error occurred
      error("write");
      return -1;
    }

    write_offset += static_cast<size_t>(w);
  }

  // If there's an open file to stream, delegate streaming to FileHandler
  if (sending_file && file_fd >= 0) {
    int r = FileHandler::streamToSocket(fd, file_fd, file_offset, file_size);
    if (r == 1) {
      // Would block, wait for EPOLLOUT
      return 1;
    } else if (r < 0) {
      error("sendfile");
      close(file_fd);
      file_fd = -1;
      sending_file = false;
      return -1;
    }

    if (file_offset >= file_size) {
      /* finished sending file */
      close(file_fd);
      file_fd = -1;
      sending_file = false;
    }
  }

  // All data sent successfully
  return 0;
}
