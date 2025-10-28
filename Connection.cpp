#include "Connection.hpp"

Connection::Connection() : fd(-1) {}

Connection::Connection(int fd) : fd(fd) {}

Connection::Connection(const Connection &other) : fd(other.fd) {}

Connection::~Connection() {}

Connection &Connection::operator=(Connection other) {
  fd = other.fd;
  return *this;
}
