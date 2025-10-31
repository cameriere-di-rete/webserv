#pragma once

#include <string>

class RequestLine {
public:
  RequestLine();
  RequestLine(const RequestLine &other);
  RequestLine &operator=(const RequestLine &other);
  ~RequestLine();

  std::string method;
  std::string uri;
  std::string version;

  std::string toString() const;
  bool parse(const std::string &line);
};
