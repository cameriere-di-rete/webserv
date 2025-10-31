#include "Message.hpp"

#include <cctype>
#include <sstream>

/* helper functions in anonymous namespace (C++98-safe) */
namespace {
std::string trim_copy(const std::string &s) {
  std::string res = s;

  // left trim
  std::string::size_type i = 0;
  while (i < res.size() &&
         (res[i] == ' ' || res[i] == '\t' || res[i] == '\r' || res[i] == '\n'))
    ++i;
  res.erase(0, i);

  // right trim
  if (!res.empty()) {
    std::string::size_type j = res.size();
    while (j > 0 && (res[j - 1] == ' ' || res[j - 1] == '\t' ||
                     res[j - 1] == '\r' || res[j - 1] == '\n'))
      --j;
    res.erase(j);
  }

  return res;
}

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
} // anonymous namespace

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
    o << it->name << ": " << it->value << "\r\n";
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
  o << startLine() << "\r\n";
  o << serializeHeaders();
  o << "\r\n";
  o << body.data;
  return o.str();
}
