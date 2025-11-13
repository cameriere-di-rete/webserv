#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Trim whitespace from both ends of a string and return the trimmed copy.
std::string trim_copy(const std::string &s) {
  std::string res = s;
  // left trim
  std::string::size_type i = 0;
  while (i < res.size() &&
         (res[i] == ' ' || res[i] == '\t' || res[i] == '\r' || res[i] == '\n'))
    ++i;
  res.erase(0, i);
  // right trim
  if (!res.empty()) {
    std::string::size_type j = res.size();
    while (j > 0 && (res[j - 1] == ' ' || res[j - 1] == '\t' ||
                     res[j - 1] == '\r' || res[j - 1] == '\n'))
      --j;
    res.erase(j);
  }
  return res;
}
