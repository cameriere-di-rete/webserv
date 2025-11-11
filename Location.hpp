#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

class Location {
public:
  Location();
  Location(const std::string &path);
  Location(const Location &other);
  Location &operator=(const Location &other);
  ~Location();
  // Types
  enum Method { GET, POST, PUT, DELETE, HEAD };

  // Location-specific configuration
  std::set<Method>
      allow_methods; // allowed methods (default: GET, POST, PUT, DELETE, HEAD)
  int redirect_code; // numeric redirect status code (e.g. 301, 302)
  std::string redirect_location; // location to redirect to
  bool cgi;                      // whether CGI is enabled for this location
  std::set<std::string> index;   // index files (e.g. index.html)
  bool autoindex;                // enable directory listing
  std::string root;              // filesystem root for this location
  std::map<int, std::string> error_page; // custom error page mapping
};
