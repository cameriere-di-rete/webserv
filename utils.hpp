
#pragma once

#include "BlockNode.hpp"

int error(const std::string &s);
int set_nonblocking(int fd);

void dumpConfig(const BlockNode &b);
