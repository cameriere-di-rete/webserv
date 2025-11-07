#include "Logger.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>

int error(const std::string &s) {
  LOG(ERROR) << s << ": " << strerror(errno);
  return EXIT_FAILURE;
}

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
