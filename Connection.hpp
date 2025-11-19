#pragma once

#include <cstddef>
#include <string>

#include "Request.hpp"
#include "Response.hpp"

class Connection {
 public:
  Connection();
  Connection(int fd);
  Connection(const Connection& other);
  ~Connection();

  Connection& operator=(const Connection& other);

  int fd;
  int server_fd;
  std::string read_buffer;
  std::string write_buffer;
  std::size_t write_offset;
  bool read_done;
  bool write_ready;
  Request request;
  Response response;

  int handleRead();
  int handleWrite();
};
