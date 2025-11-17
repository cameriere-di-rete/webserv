#include "StatusLine.hpp"
#include "HttpStatus.hpp"
#include "constants.hpp"
#include <sstream>

StatusLine::StatusLine()
    : version(HTTP_VERSION), status_code(http::S_200_OK),
      reason(http::reasonPhrase(http::S_200_OK)) {}

StatusLine::StatusLine(const StatusLine &other)
    : version(other.version), status_code(other.status_code),
      reason(other.reason) {}

StatusLine &StatusLine::operator=(const StatusLine &other) {
  if (this != &other) {
    version = other.version;
    status_code = other.status_code;
    reason = other.reason;
  }
  return *this;
}

StatusLine::~StatusLine() {}

std::string StatusLine::toString() const {
  std::ostringstream o;
  o << version << " " << status_code << " " << reason;
  return o.str();
}

bool StatusLine::parse(const std::string &line) {
  std::istringstream in(line);
  int code = 0;
  if (!(in >> version >> code)) {
    return false;
  }
  try {
    status_code = http::intToStatus(code);
  } catch (const std::invalid_argument &) {
    return false;
  }
  std::getline(in, reason);
  if (!reason.empty() && reason[0] == ' ') {
    reason.erase(0, 1);
  }
  return true;
}
