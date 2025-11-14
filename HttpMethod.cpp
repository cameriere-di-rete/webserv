#include "HttpMethod.hpp"
#include <stdexcept>

namespace http {

std::string methodToString(Method m) {
  switch (m) {
  case GET:
    return "GET";
  case POST:
    return "POST";
  case PUT:
    return "PUT";
  case DELETE:
    return "DELETE";
  case HEAD:
    return "HEAD";
  default:
    return "UNKNOWN";
  }
}

Method stringToMethod(const std::string &s) {
  if (s == "GET") {
    return GET;
  }
  if (s == "POST") {
    return POST;
  }
  if (s == "PUT") {
    return PUT;
  }
  if (s == "DELETE") {
    return DELETE;
  }
  if (s == "HEAD") {
    return HEAD;
  }
  throw std::invalid_argument(std::string("Unknown HTTP method: ") + s);
}

} // namespace http
