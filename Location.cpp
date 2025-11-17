#include "Location.hpp"
#include "Logger.hpp"
#include "utils.hpp"

Location::Location()
    : path(), allow_methods(), redirect_code(http::UNKNOWN),
      redirect_location(), cgi(false), index(), autoindex(false), root(),
      error_page() {
  LOG(DEBUG) << "Location() default constructor called";
  initDefaultHttpMethods(allow_methods);
  LOG(DEBUG)
      << "Location initialized with default allowed methods (GET, POST, PUT, "
         "DELETE, HEAD)";
}

Location::Location(const std::string &p)
    : path(p), allow_methods(), redirect_code(http::UNKNOWN),
      redirect_location(), cgi(false), index(), autoindex(false), root(),
      error_page() {
  LOG(DEBUG) << "Location(path) constructor called with path: " << p;
  initDefaultHttpMethods(allow_methods);
  LOG(DEBUG) << "Location '" << path
             << "' initialized with default allowed methods";
}

Location::Location(const Location &other)
    : path(other.path), allow_methods(other.allow_methods),
      redirect_code(other.redirect_code),
      redirect_location(other.redirect_location), cgi(other.cgi),
      index(other.index), autoindex(other.autoindex), root(other.root),
      error_page(other.error_page) {}

Location &Location::operator=(const Location &other) {
  if (this != &other) {
    path = other.path;
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
