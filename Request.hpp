#pragma once

#include "Message.hpp"
#include "RequestLine.hpp"

class Request : public Message {
 public:
  Request();
  Request(const Request& other);
  Request& operator=(const Request& other);
  virtual ~Request();

  RequestLine request_line;

  virtual std::string startLine() const;
  bool parseStartAndHeaders(const std::vector<std::string>& lines);
};
