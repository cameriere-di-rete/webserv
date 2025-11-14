#pragma once

#include <map>
#include <set>
#include <string>

class Location {
public:
  Location();
  Location(const std::string &path);
  Location(const Location &other);
  Location &operator=(const Location &other);
  ~Location();

  // Types
  enum Method { GET, POST, PUT, DELETE, HEAD };

  // Location path identifier
  std::string path;

  // Location-specific configuration
  std::set<Method> allow_methods;
  int redirect_code;
  std::string redirect_location;
  bool cgi;
  std::set<std::string> index;
  bool autoindex;
  std::string root;
  std::map<int, std::string> error_page;
};
