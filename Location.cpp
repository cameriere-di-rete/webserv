#include "Location.hpp"

Location::Location()
    : allow_methods(), redirect_code(0), redirect_location(), cgi(false),
      index(), autoindex(false), root(), error_page() {
  // default allowed methods
  allow_methods.insert(GET);
  allow_methods.insert(POST);
  allow_methods.insert(PUT);
  allow_methods.insert(DELETE);
  allow_methods.insert(HEAD);
}

Location::Location(const std::string &p)
    : allow_methods(), redirect_code(0), redirect_location(), cgi(false),
      index(), autoindex(false), root(), error_page() {
  // default allowed methods
  allow_methods.insert(GET);
  allow_methods.insert(POST);
  allow_methods.insert(PUT);
  allow_methods.insert(DELETE);
  allow_methods.insert(HEAD);
}

Location::Location(const Location &other)
    : allow_methods(other.allow_methods), redirect_code(other.redirect_code),
      redirect_location(other.redirect_location), cgi(other.cgi),
      index(other.index), autoindex(other.autoindex), root(other.root),
      error_page(other.error_page) {}

Location &Location::operator=(const Location &other) {
  if (this != &other) {
    allow_methods = other.allow_methods;
    redirect_code = other.redirect_code;
    redirect_location = other.redirect_location;
    cgi = other.cgi;
    index = other.index;
    autoindex = other.autoindex;
    root = other.root;
    error_page = other.error_page;
  }
  return *this;
}

Location::~Location() {}
