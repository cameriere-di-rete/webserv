
#pragma once

#include <sys/types.h>
#include <stdint.h>

int set_nonblocking(int fd);
void update_events(int efd, int fd, uint32_t events);
