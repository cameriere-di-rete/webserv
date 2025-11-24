#pragma once

#include <cstddef>
#include <string>

#include "HttpStatus.hpp"
#include "Request.hpp"
#include "Response.hpp"

class Connection {
 public:
  Connection();
  Connection(int file_descriptor);
  Connection(const Connection& other);
  ~Connection();

  Connection& operator=(const Connection& other);

  int fd;
  int server_fd;
  std::string read_buffer;
  std::string write_buffer;
  std::size_t write_offset;
  std::size_t headers_end_pos;
  bool write_ready;
  Request request;
  Response response;

  int handleRead();
  int handleWrite();
  void processRequest(const class Server& server);
  void processResponse(const class Location& location);
  void prepareErrorResponse(http::Status status);
};
