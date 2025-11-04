
#pragma once

#include <string>

int error(const char *s);
int set_nonblocking(int fd);

// Read entire file into a std::string. Throws std::runtime_error if the file
// can't be opened.
std::string readFile(const std::string &path);

// Debug helper: print the parsed configuration tree (uses parsecfg AST)
#include "Parse_Config.hpp"
void dumpConfig(const parsecfg::BlockNode &b);

// Print an error message to stderr in red color
void printError(const std::string &msg);
