#include "Header.hpp"

Header::Header() : name(), value() {}

Header::Header(const std::string& n, const std::string& v)
    : name(n), value(v) {}

Header::Header(const Header& other) : name(other.name), value(other.value) {}

Header& Header::operator=(const Header& other) {
  if (this != &other) {
    name = other.name;
    value = other.value;
  }
  return *this;
}

Header::~Header() {}
