#pragma once

#include <string>

class StatusLine {
public:
  StatusLine();
  StatusLine(const StatusLine &other);
  StatusLine &operator=(const StatusLine &other);
  ~StatusLine();

  std::string version;
  int status_code;
  std::string reason;

  std::string toString() const;
  bool parse(const std::string &line);
};
