#include "Message.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <cctype>
#include <sstream>

namespace {
bool ci_equal_copy(const std::string &a, const std::string &b) {
  if (a.size() != b.size())
    return false;
  for (std::string::size_type i = 0; i < a.size(); ++i) {
    char ca = a[i];
    char cb = b[i];
    ca = static_cast<char>(std::tolower(static_cast<unsigned char>(ca)));
    cb = static_cast<char>(std::tolower(static_cast<unsigned char>(cb)));
    if (ca != cb)
      return false;
  }
  return true;
}
} // namespace

/* Message */
Message::Message() : headers(), body() {}

Message::Message(const Message &other)
    : headers(other.headers), body(other.body) {}

Message &Message::operator=(const Message &other) {
  if (this != &other) {
    headers = other.headers;
    body = other.body;
  }
  return *this;
}

Message::~Message() {}

void Message::addHeader(const std::string &name, const std::string &value) {
  headers.push_back(Header(name, value));
}

bool Message::getHeader(const std::string &name, std::string &out) const {
  for (std::vector<Header>::const_iterator it = headers.begin();
       it != headers.end(); ++it) {
    if (ci_equal_copy(it->name, name)) {
      out = it->value;
      return true;
    }
  }
  return false;
}

std::vector<std::string> Message::getHeaders(const std::string &name) const {
  std::vector<std::string> res;
  for (std::vector<Header>::const_iterator it = headers.begin();
       it != headers.end(); ++it) {
    if (ci_equal_copy(it->name, name))
      res.push_back(it->value);
  }
  return res;
}

void Message::setBody(const Body &b) {
  body = b;
}

Body &Message::getBody() {
  return body;
}

const Body &Message::getBody() const {
  return body;
}

std::string Message::serializeHeaders() const {
  std::ostringstream o;
  for (std::vector<Header>::const_iterator it = headers.begin();
       it != headers.end(); ++it) {
    o << it->name << ": " << it->value << CRLF;
  }
  return o.str();
}

bool Message::parseHeaderLine(const std::string &line, Header &out) {
  std::string::size_type pos = line.find(':');
  if (pos == std::string::npos)
    return false;
  out.name = trim_copy(line.substr(0, pos));
  out.value = trim_copy(line.substr(pos + 1));
  return true;
}

std::string Message::serialize() const {
  std::ostringstream o;
  o << startLine() << CRLF;
  o << serializeHeaders();
  o << CRLF;
  o << body.data;
  return o.str();
}

std::size_t Message::parseHeaders(const std::vector<std::string> &lines,
                                  std::size_t start) {
  std::size_t count = 0;
  for (std::vector<std::string>::size_type i = start; i < lines.size(); ++i) {
    const std::string &ln = lines[i];
    if (ln.empty())
      continue;
    Header h;
    if (parseHeaderLine(ln, h)) {
      headers.push_back(h);
      ++count;
    }
  }
  return count;
}
