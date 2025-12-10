#pragma once

#include <cstddef>
#include <map>
#include <set>
#include <string>

#include "HttpMethod.hpp"
#include "HttpStatus.hpp"

// Tri-state for boolean directives that need to distinguish "not set"
enum Tristate { UNSET = -1, OFF = 0, ON = 1 };

// Sentinel value for max_request_body meaning "not set in config"
extern const std::size_t MAX_REQUEST_BODY_UNSET;
// Default value when no max_request_body is specified anywhere
extern const std::size_t DEFAULT_MAX_REQUEST_BODY;
class Location {
 public:
  Location();
  Location(const std::string& path);
  Location(const Location& other);
  Location& operator=(const Location& other);
  ~Location();

  // Location path identifier
  std::string path;

  // Location-specific configuration
  std::set<http::Method> allow_methods;
  http::Status redirect_code;
  std::string redirect_location;
  bool cgi;
  std::set<std::string> index;
  Tristate autoindex;
  std::string root;
  std::map<http::Status, std::string> error_page;
  std::size_t max_request_body;
};
