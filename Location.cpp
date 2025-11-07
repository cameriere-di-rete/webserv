#include "Location.hpp"

Location::Location() : path(), directives() {}

Location::Location(const std::string &p) : path(p), directives() {}

Location::Location(const Location &other)
    : path(other.path), directives(other.directives) {}

Location &Location::operator=(const Location &other) {
  if (this != &other) {
    path = other.path;
    directives = other.directives;
  }
  return *this;
}

Location::~Location() {}
