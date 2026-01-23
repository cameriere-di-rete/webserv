#pragma once

#include "Message.hpp"
#include "StatusLine.hpp"

class Response : public Message {
 public:
  Response();
  Response(const Response& other);
  Response& operator=(const Response& other);
  virtual ~Response();

  StatusLine status_line;

  virtual std::string startLine() const;
  virtual std::string serialize() const;
  bool parseStartAndHeaders(const std::vector<std::string>& lines);

  // Serialize including implicit Connection header when absent
  std::string serializeHeadersWithConnection() const;

  // Helper methods to reduce boilerplate when constructing responses
  void setStatus(http::Status status, const std::string& version);
  void setBodyWithContentType(const std::string& data,
                              const std::string& contentType);
  // Add a Set-Cookie header. `attrs` can contain semicolon-separated
  // attributes like "Path=/; HttpOnly".
  void addCookie(const std::string& name, const std::string& value,
                 const std::string& attrs = "");
};
