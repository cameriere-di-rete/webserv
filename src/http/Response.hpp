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
  bool parseStartAndHeaders(const std::vector<std::string>& lines);

  // Helper methods to reduce boilerplate when constructing responses
  void setStatus(http::Status status, const std::string& version);
  void setBodyWithContentType(const std::string& data,
                              const std::string& contentType);
};
