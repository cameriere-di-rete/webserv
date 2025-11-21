#include "RequestLine.hpp"

#include <sstream>

RequestLine::RequestLine() {}

RequestLine::RequestLine(const RequestLine& other)
    : method(other.method), uri(other.uri), version(other.version) {}

RequestLine& RequestLine::operator=(const RequestLine& other) {
  if (this != &other) {
    method = other.method;
    uri = other.uri;
    version = other.version;
  }
  return *this;
}

RequestLine::~RequestLine() {}

std::string RequestLine::toString() const {
  std::ostringstream o;
  o << method << " " << uri << " " << version;
  return o.str();
}

bool RequestLine::parse(const std::string& line) {
  std::istringstream in(line);
  return !!(in >> method >> uri >> version);
}
