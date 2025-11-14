#include "HttpStatus.hpp"
#include <sstream>
#include <stdexcept>

namespace http {

std::string reasonPhrase(Status status) {
  switch (status) {
  case 200:
    return "OK";
  case 400:
    return "Bad Request";
  case 404:
    return "Not Found";
  case 500:
    return "Internal Server Error";
  case 201:
    return "Created";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 303:
    return "See Other";
  case 307:
    return "Temporary Redirect";
  case 308:
    return "Permanent Redirect";
  case 401:
    return "Unauthorized";
  case 403:
    return "Forbidden";
  case 405:
    return "Method Not Allowed";
  case 413:
    return "Payload Too Large";
  case 414:
    return "URI Too Long";
  case 501:
    return "Not Implemented";
  case 502:
    return "Bad Gateway";
  case 503:
    return "Service Unavailable";
  case 504:
    return "Gateway Timeout";
  default:
    return "";
  }
}

Status intToStatus(int status) {
  switch (status) {
  case 200:
    return OK;
  case 201:
    return CREATED;
  case 204:
    return NO_CONTENT;
  case 301:
    return MOVED_PERMANENTLY;
  case 302:
    return FOUND;
  case 303:
    return SEE_OTHER;
  case 307:
    return TEMPORARY_REDIRECT;
  case 308:
    return PERMANENT_REDIRECT;
  case 400:
    return BAD_REQUEST;
  case 401:
    return UNAUTHORIZED;
  case 403:
    return FORBIDDEN;
  case 404:
    return NOT_FOUND;
  case 405:
    return METHOD_NOT_ALLOWED;
  case 413:
    return PAYLOAD_TOO_LARGE;
  case 414:
    return URI_TOO_LONG;
  case 500:
    return INTERNAL_SERVER_ERROR;
  case 501:
    return NOT_IMPLEMENTED;
  case 502:
    return BAD_GATEWAY;
  case 503:
    return SERVICE_UNAVAILABLE;
  case 504:
    return GATEWAY_TIMEOUT;
  default:
    throw std::invalid_argument("Unknown HTTP status code");
  }
}

std::string statusWithReason(Status s) {
  std::ostringstream oss;
  oss << s;
  std::string reason = reasonPhrase(s);
  if (!reason.empty()) {
    oss << " " << reason;
  }
  return oss.str();
}

bool isSuccess(int status) {
  try {
    Status s = intToStatus(status);
    int code = static_cast<int>(s);
    return code >= 200 && code <= 299;
  } catch (const std::invalid_argument &) {
    return false;
  }
}

bool isRedirect(int status) {
  try {
    Status s = intToStatus(status);
    int code = static_cast<int>(s);
    return code >= 300 && code <= 399;
  } catch (const std::invalid_argument &) {
    return false;
  }
}

bool isClientError(int status) {
  try {
    Status s = intToStatus(status);
    int code = static_cast<int>(s);
    return code >= 400 && code <= 499;
  } catch (const std::invalid_argument &) {
    return false;
  }
}

bool isServerError(int status) {
  try {
    Status s = intToStatus(status);
    int code = static_cast<int>(s);
    return code >= 500 && code <= 599;
  } catch (const std::invalid_argument &) {
    return false;
  }
}

bool isValidStatusCode(int status) {
  try {
    (void)intToStatus(status);
    return true;
  } catch (const std::invalid_argument &) {
    return false;
  }
}

} // namespace http
