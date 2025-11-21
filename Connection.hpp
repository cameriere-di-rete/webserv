#pragma once

#include <cstddef>
#include <string>
#include <sys/types.h>

#include "HttpStatus.hpp"
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
  std::size_t headers_end_pos;
  bool write_ready;
  Request request;
  Response response;
  /* file streaming state (used when serving files without loading into memory)
   */
  int file_fd;
  off_t file_offset;
  off_t file_size;
  bool sending_file;

  int handleRead();
  int handleWrite();
  void processRequest(const class Server& server);
  void processResponse(const class Location& location);
  void prepareErrorResponse(http::Status status);
};
