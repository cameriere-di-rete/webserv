#include "Request.hpp"

Request::Request() : Message(), request_line() {}

Request::Request(const Request &other)
    : Message(other), request_line(other.request_line) {}

Request &Request::operator=(const Request &other) {
  if (this != &other) {
    Message::operator=(other);
    request_line = other.request_line;
  }
  return *this;
}

Request::~Request() {}

std::string Request::startLine() const {
  return request_line.toString();
}

bool Request::parseStartAndHeaders(const std::vector<std::string> &lines) {
  if (lines.empty())
    return false;
  if (!request_line.parse(lines[0]))
    return false;
  for (std::vector<std::string>::size_type i = 1; i < lines.size(); ++i) {
    const std::string &ln = lines[i];
    if (ln.empty())
      continue;
    Header h;
    if (parseHeaderLine(ln, h))
      headers.push_back(h);
  }
  return true;
}
