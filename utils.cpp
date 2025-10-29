#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/epoll.h>

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void update_events(int efd, int fd, uint32_t events) {
  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev) == -1) {
    if (errno == ENOENT) {
      /* fd not yet in the set → add it */
      if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) == -1)
        throw strerror(errno);
    } else {
      throw strerror(errno);
    }
  }
}
