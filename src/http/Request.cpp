#include "Request.hpp"

#include "utils.hpp"

Request::Request() : Message(), request_line(), uri() {}

Request::Request(const Request& other)
    : Message(other), request_line(other.request_line), uri(other.uri) {}

Request& Request::operator=(const Request& other) {
  if (this != &other) {
    Message::operator=(other);
    request_line = other.request_line;
    uri = other.uri;
  }
  return *this;
}

Request::~Request() {}

std::string Request::startLine() const {
  return request_line.toString();
}

bool Request::parseStartAndHeaders(const std::string& buffer,
                                   std::size_t headers_pos) {
  if (headers_pos == std::string::npos) {
    return false;
  }

  std::vector<std::string> lines;
  std::string temp;
  for (std::size_t i = 0; i < headers_pos; ++i) {
    char ch = buffer[i];
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      lines.push_back(temp);
      temp.clear();
    } else {
      temp.push_back(ch);
    }
  }

  if (!temp.empty()) {
    lines.push_back(temp);
  }

  if (lines.empty()) {
    return false;
  }

  if (!request_line.parse(lines[0])) {
    return false;
  }

  // Parse the URI from request_line.uri
  if (!uri.parse(request_line.uri)) {
    return false;
  }

  parseHeaders(lines, 1);
  // Parse Cookie headers into the cookies map
  std::vector<std::string> cookie_headers = getHeaders("Cookie");
  for (std::vector<std::string>::const_iterator it = cookie_headers.begin();
       it != cookie_headers.end(); ++it) {
    const std::string& ch = *it;
    std::string::size_type start = 0;
    while (start < ch.size()) {
      std::string::size_type sep = ch.find(';', start);
      std::string pair = (sep == std::string::npos)
                             ? ch.substr(start)
                             : ch.substr(start, sep - start);
      pair = trim_copy(pair);
      std::string::size_type eq = pair.find('=');
      if (eq != std::string::npos) {
        std::string k = trim_copy(pair.substr(0, eq));
        std::string v = trim_copy(pair.substr(eq + 1));
        if (!k.empty()) {
          // If multiple cookies share the same name, the last occurrence
          // overwrites the previous value ("last value wins" policy as defined
          // by the application).
          cookies[k] = v;
        }
      }
      if (sep == std::string::npos) {
        break;
      }
      start = sep + 1;
    }
  }
  return true;
}

bool Request::getCookie(const std::string& name, std::string& out) const {
  std::map<std::string, std::string>::const_iterator it = cookies.find(name);
  if (it == cookies.end()) {
    return false;
  }
  out = it->second;
  return true;
}
