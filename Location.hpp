#pragma once

#include <map>
#include <string>
#include <vector>

class Location {
public:
  Location();
  Location(const std::string &path);
  Location(const Location &other);
  Location &operator=(const Location &other);
  ~Location();

  std::string path;
  std::map<std::string, std::vector<std::string> > directives;
};
