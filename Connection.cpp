#include "Connection.hpp"

Connection::Connection() : fd(-1), write_offset(0) {}

Connection::Connection(int fd) : fd(fd), write_offset(0) {}

Connection::Connection(const Connection &other) : fd(other.fd), write_offset(0) {}

Connection::~Connection() {}

Connection &Connection::operator=(Connection other) {
  fd = other.fd;
  return *this;
}
