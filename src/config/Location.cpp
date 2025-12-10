#include "Location.hpp"

#include "Logger.hpp"

// Definition of the sentinel values declared in Location.hpp
const std::size_t MAX_REQUEST_BODY_UNSET = static_cast<std::size_t>(-1);
const std::size_t DEFAULT_MAX_REQUEST_BODY = 4096;

Location::Location()
    : path(),
      allow_methods(),
      redirect_code(http::S_0_UNKNOWN),
      redirect_location(),
      cgi(false),
      index(),
      autoindex(UNSET),
      root(),
      error_page(),
      max_request_body(MAX_REQUEST_BODY_UNSET) {
  LOG(DEBUG) << "Location() default constructor called";
}

Location::Location(const std::string& p)
    : path(p),
      allow_methods(),
      redirect_code(http::S_0_UNKNOWN),
      redirect_location(),
      cgi(false),
      index(),
      autoindex(UNSET),
      root(),
      error_page(),
      max_request_body(MAX_REQUEST_BODY_UNSET) {
  LOG(DEBUG) << "Location(path) constructor called with path: " << p;
}

Location::Location(const Location& other)
    : path(other.path),
      allow_methods(other.allow_methods),
      redirect_code(other.redirect_code),
      redirect_location(other.redirect_location),
      cgi(other.cgi),
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
    cgi = other.cgi;
    index = other.index;
    autoindex = other.autoindex;
    root = other.root;
    error_page = other.error_page;
    max_request_body = other.max_request_body;
  }
  return *this;
}

Location::~Location() {}
