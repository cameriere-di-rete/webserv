#include "StatusLine.hpp"
#include "constants.hpp"
#include <sstream>

StatusLine::StatusLine()
    : version(HTTP_VERSION), status_code(200), reason("OK") {}

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
  if (!(in >> version >> status_code))
    return false;
  std::getline(in, reason);
  if (!reason.empty() && reason[0] == ' ')
    reason.erase(0, 1);
  return true;
}
