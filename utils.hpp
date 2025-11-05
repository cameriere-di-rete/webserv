
#pragma once

#include "BlockNode.hpp"
#include <string>

int error(const char *s);
int set_nonblocking(int fd);

void dumpConfig(const BlockNode &b);

// Print an error message to stderr in red color
void printError(const std::string &msg);
