#include "Connection.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/socket.h>

Connection::Connection()
    : fd(-1), write_offset(0), read_done(false), write_ready(false) {}

Connection::Connection(int fd)
    : fd(fd), write_offset(0), read_done(false), write_ready(false) {}

Connection::Connection(const Connection &other)
    : fd(other.fd), write_buffer(other.write_buffer),
      write_offset(other.write_offset) {}

Connection::~Connection() {}

Connection &Connection::operator=(const Connection &other) {
  if (this != &other) {
    fd = other.fd;
    read_buffer = other.read_buffer;
    read_done = other.read_done;
    write_buffer = other.write_buffer;
    write_offset = other.write_offset;
    write_ready = other.write_ready;
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

    // Aggiungo i nuovi dati al buffer persistente
    read_buffer.append(buf, r);

    // Controllo se la richiesta HTTP è completa
    if (read_buffer.find("\r\n\r\n") != std::string::npos) {
      std::cout << "=== HTTP request received ===" << std::endl;
      std::cout << "File descriptor: " << fd << std::endl;
      std::cout << "Bytes received: " << r << std::endl;
      std::cout << "Content:" << std::endl;
      std::cout << read_buffer << std::endl;
      std::cout << "===========================" << std::endl;
      write_buffer = "HTTP/1.0 200 OK" CRLF "Content-Type: text/plain; "
                     "charset=utf-8" CRLF CRLF;
      write_buffer.append(read_buffer);
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

  // All data sent successfully
  return 0;
}
