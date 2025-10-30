#pragma once

#include <string>
class StatusLine {
public:
  StatusLine();
  StatusLine(const StatusLine &other);
  StatusLine &operator=(const StatusLine &other);
  ~StatusLine();

  std::string version; // "HTTP/1.1"
  int status_code;     // 200
  std::string reason;  // "OK"

  std::string toString() const;
  bool parse(const std::string &line);
};
