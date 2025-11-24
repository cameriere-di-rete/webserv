#pragma once

#include <sys/types.h>

#include <cstddef>
#include <string>

#include "HttpStatus.hpp"
#include "IHandler.hpp"
#include "Request.hpp"
#include "Response.hpp"

// forward declare for handler pointer ownership
class IHandler;

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
  std::size_t headers_end_pos;
  bool write_ready;
  Request request;
  Response response;
  // active request handler (if any)
  IHandler* active_handler;

  int handleRead();
  int handleWrite();
  void processRequest(const class Server& server);
  void processResponse(const class Location& location);
  void prepareErrorResponse(http::Status status);
  // handler ownership helpers
  void setHandler(IHandler* h);
  void clearHandler();
};
