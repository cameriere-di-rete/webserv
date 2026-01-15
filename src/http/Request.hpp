#pragma once

#include <map>
#include <string>

#include "Message.hpp"
#include "RequestLine.hpp"
#include "Uri.hpp"

class Request : public Message {
 public:
  Request();
  Request(const Request& other);
  Request& operator=(const Request& other);
  virtual ~Request();

  RequestLine request_line;
  http::Uri uri;  // Parsed URI from request_line.uri
  // Parsed cookies from Cookie headers (name -> value)
  std::map<std::string, std::string> cookies;

  // Retrieve a cookie value by name. Returns true if found.
  bool getCookie(const std::string& name, std::string& out) const;

  virtual std::string startLine() const;
  bool parseStartAndHeaders(const std::string& buffer, std::size_t headers_pos);
};
