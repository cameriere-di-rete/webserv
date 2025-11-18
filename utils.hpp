
#pragma once

#include <set>
#include <string>

#include "HttpMethod.hpp"

int set_nonblocking(int fd);

// Trim whitespace (space, tab, CR, LF) from both ends of a string.
// Returns a copy with the trimmed content.
std::string trim_copy(const std::string& s);

// Initialize a set with the default allowed HTTP methods
// (GET, POST, PUT, DELETE, HEAD)
void initDefaultHttpMethods(std::set<http::Method>& methods);
