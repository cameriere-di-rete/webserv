#include "Connection.hpp"

Connection::Connection() : fd(-1), write_offset(0) {}

Connection::Connection(int fd) : fd(fd), write_offset(0) {}

Connection::Connection(const Connection &other)
    : fd(other.fd), in(other.in), write_offset(0) {}

Connection::~Connection() {}

Connection &Connection::operator=(Connection other) {
  if (this != &other) {
    fd = other.fd;
    in = other.in;
    write_offset = other.write_offset;
  }
  return *this;
}
