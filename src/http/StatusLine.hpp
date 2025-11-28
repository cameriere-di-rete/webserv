#pragma once

#include <string>

#include "HttpStatus.hpp"

class StatusLine {
 public:
  StatusLine();
  StatusLine(const StatusLine& other);
  StatusLine& operator=(const StatusLine& other);
  ~StatusLine();

  std::string version;
  http::Status status_code;
  std::string reason;

  std::string toString() const;
  bool parse(const std::string& line);
};
