
#pragma once

#include <sys/types.h>

int set_nonblocking(int fd);
void update_events(int efd, int fd, u_int32_t events);
