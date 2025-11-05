
#pragma once

#include <string>

int error(const char *s);
int set_nonblocking(int fd);

// Debug helper: print the parsed configuration tree (uses parsecfg AST)
#include "Config.hpp"
void dumpConfig(const BlockNode &b);

// Print an error message to stderr in red color
void printError(const std::string &msg);
