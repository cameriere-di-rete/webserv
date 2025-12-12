#include "Location.hpp"

#include "Logger.hpp"

Location::Location()
    : path(),
      allow_methods(),
      redirect_code(http::S_0_UNKNOWN),
      redirect_location(),
      cgi_root(),
      index(),
      autoindex(UNSET),
      root(),
      error_page(),
      max_request_body(0) {
  LOG(DEBUG) << "Location() default constructor called";
}

Location::Location(const std::string& path_str)
    : path(path_str),
      redirect_code(http::S_0_UNKNOWN),
      redirect_location(),
      cgi_root(),
      index(),
      autoindex(UNSET),
      root(),
      error_page(),
      max_request_body(0) {
  LOG(DEBUG) << "Location(path) constructor called with path: " << path_str;
}

Location::Location(const Location& other)
    : path(other.path),
      allow_methods(other.allow_methods),
      redirect_code(other.redirect_code),
      redirect_location(other.redirect_location),
      cgi_root(other.cgi_root),
      index(other.index),
      autoindex(other.autoindex),
      root(other.root),
      error_page(other.error_page),
      max_request_body(other.max_request_body) {}

Location& Location::operator=(const Location& other) {
  if (this != &other) {
    path = other.path;
    allow_methods = other.allow_methods;
    redirect_code = other.redirect_code;
    redirect_location = other.redirect_location;
    cgi_root = other.cgi_root;
    index = other.index;
    autoindex = other.autoindex;
    root = other.root;
    error_page = other.error_page;
    max_request_body = other.max_request_body;
  }
  return *this;
}

Location::~Location() {}
