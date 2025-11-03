
#pragma once

#include <string>

int error(const char *s);
int set_nonblocking(int fd);

// Read entire file into a std::string. Throws std::runtime_error if the file
// can't be opened.
std::string readFile(const std::string &path);

// Debug helper: print the parsed configuration tree
struct Block; // forward-declare to avoid including Parser.hpp here
void dumpConfig(const Block &b);
