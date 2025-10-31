#pragma once

#include "Message.hpp"
#include "StatusLine.hpp"

class Response : public Message {
public:
  Response();
  Response(const Response &other);
  Response &operator=(const Response &other);
  virtual ~Response();

  StatusLine status_line;

  virtual std::string startLine() const;
  bool parseStartAndHeaders(const std::vector<std::string> &lines);
};
