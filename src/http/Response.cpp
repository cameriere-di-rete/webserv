#include "Response.hpp"

#include <sstream>

#include "HttpStatus.hpp"

Response::Response() : Message(), status_line() {}

Response::Response(const Response& other)
    : Message(other), status_line(other.status_line) {}

Response& Response::operator=(const Response& other) {
  if (this != &other) {
    Message::operator=(other);
    status_line = other.status_line;
  }

  return *this;
}

Response::~Response() {}

std::string Response::startLine() const {
  return status_line.toString();
}

bool Response::parseStartAndHeaders(const std::vector<std::string>& lines) {
  if (lines.empty()) {
    return false;
  }
  if (!status_line.parse(lines[0])) {
    return false;
  }
  parseHeaders(lines, 1);
  return true;
}

void Response::setStatus(http::Status status, const std::string& version) {
  status_line.version = version;
  status_line.status_code = status;
  status_line.reason = http::reasonPhrase(status);
}

void Response::setBodyWithContentType(const std::string& data,
                                      const std::string& contentType) {
  body.data = data;
  addHeader("Content-Type", contentType);
  std::ostringstream oss;
  oss << body.size();
  addHeader("Content-Length", oss.str());
}

void Response::addCookie(const std::string& name, const std::string& value,
                         const std::string& attrs) {
  std::string cookie = name + "=" + value;
  if (!attrs.empty()) {
    cookie += "; ";
    cookie += attrs;
  }
  addHeader("Set-Cookie", cookie);
}
