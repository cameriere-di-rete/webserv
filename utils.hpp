
#pragma once

#include <string>

int set_nonblocking(int fd);

// Trim whitespace (space, tab, CR, LF) from both ends of a string.
// Returns a copy with the trimmed content.
std::string trim_copy(const std::string &s);
