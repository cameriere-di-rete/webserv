#pragma once

#include <cstddef>
#include <string>

class Connection {
public:
  Connection();
  Connection(int fd);
  Connection(const Connection &other);
  ~Connection();

  Connection &operator=(const Connection &other);

  int fd;
  std::string write_buffer;
  std::size_t write_offset;
};
