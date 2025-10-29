
#pragma once

#include <stdint.h>
#include <sys/types.h>

int set_nonblocking(int fd);
void update_events(int efd, int fd, uint32_t events);
