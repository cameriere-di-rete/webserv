#pragma once

#include <string>

class Header {
private:
public:
  Header();
  Header(const std::string &n, const std::string &v);
  Header(const Header &other);
  Header &operator=(const Header &other);
  ~Header();

  std::string name;
  std::string value;
};
