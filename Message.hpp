#pragma once

#include "Body.hpp"
#include "Header.hpp"
#include <string>
#include <vector>

class Message {
public:
  Message();
  Message(const Message &other);
  Message &operator=(const Message &other);
  virtual ~Message();

  void addHeader(const std::string &name, const std::string &value);
  bool getHeader(const std::string &name, std::string &out) const;
  std::vector<std::string> getHeaders(const std::string &name) const;

  void setBody(const Body &b);
  Body &getBody();
  const Body &getBody() const;

  std::string serializeHeaders() const;
  static bool parseHeaderLine(const std::string &line, Header &out);

  virtual std::string startLine() const = 0;
  virtual std::string serialize() const;

protected:
  std::vector<Header> headers;
  Body body;
};
