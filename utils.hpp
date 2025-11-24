
#pragma once

#include <set>
#include <string>

#include "HttpMethod.hpp"

int set_nonblocking(int file_descriptor);

// Trim whitespace (space, tab, CR, LF) from both ends of a string.
// Returns a copy with the trimmed content.
std::string trim_copy(const std::string& str);

// Initialize a set with the default allowed HTTP methods
// (GET, POST, PUT, DELETE, HEAD)
void initDefaultHttpMethods(std::set<http::Method>& methods);

// Parse log level flag (e.g., "-l:0" for DEBUG, "-l:1" for INFO, "-l:2" for
// ERROR)
int parseLogLevelFlag(const std::string& arg);

// Parse program arguments and fill `path` and `logLevel`.
// This was moved out of main to keep main shorter and clearer.
void processArgs(int argc, char** argv, std::string& path, int& logLevel);
