#include "HttpStatus.hpp"

#include <sstream>
#include <stdexcept>

namespace http {

std::string reasonPhrase(Status status) {
  switch (status) {
    case S_200_OK:
      return "OK";
    case S_201_CREATED:
      return "Created";
    case S_204_NO_CONTENT:
      return "No Content";
    case S_206_PARTIAL_CONTENT:
      return "Partial Content";
    case S_301_MOVED_PERMANENTLY:
      return "Moved Permanently";
    case S_302_FOUND:
      return "Found";
    case S_303_SEE_OTHER:
      return "See Other";
    case S_307_TEMPORARY_REDIRECT:
      return "Temporary Redirect";
    case S_308_PERMANENT_REDIRECT:
      return "Permanent Redirect";
    case S_400_BAD_REQUEST:
      return "Bad Request";
    case S_401_UNAUTHORIZED:
      return "Unauthorized";
    case S_402_PAYMENT_REQUIRED:
      return "Payment Required";
    case S_403_FORBIDDEN:
      return "Forbidden";
    case S_404_NOT_FOUND:
      return "Not Found";
    case S_405_METHOD_NOT_ALLOWED:
      return "Method Not Allowed";
    case S_406_NOT_ACCEPTABLE:
      return "Not Acceptable";
    case S_408_REQUEST_TIMEOUT:
      return "Request Timeout";
    case S_409_CONFLICT:
      return "Conflict";
    case S_410_GONE:
      return "Gone";
    case S_411_LENGTH_REQUIRED:
      return "Length Required";
    case S_413_PAYLOAD_TOO_LARGE:
      return "Payload Too Large";
    case S_414_URI_TOO_LONG:
      return "URI Too Long";
    case S_415_UNSUPPORTED_MEDIA_TYPE:
      return "Unsupported Media Type";
    case S_416_RANGE_NOT_SATISFIABLE:
      return "Range Not Satisfiable";
    case S_417_EXPECTATION_FAILED:
      return "Expectation Failed";
    case S_418_IM_A_TEAPOT:
      return "I'm a teapot";
    case S_426_UPGRADE_REQUIRED:
      return "Upgrade Required";
    case S_428_PRECONDITION_REQUIRED:
      return "Precondition Required";
    case S_429_TOO_MANY_REQUESTS:
      return "Too Many Requests";
    case S_431_REQUEST_HEADER_FIELDS_TOO_LARGE:
      return "Header Fields Too Large";
    case S_451_UNAVAILABLE_FOR_LEGAL_REASONS:
      return "Legal Reasons";
    case S_500_INTERNAL_SERVER_ERROR:
      return "Internal Server Error";
    case S_501_NOT_IMPLEMENTED:
      return "Not Implemented";
    case S_502_BAD_GATEWAY:
      return "Bad Gateway";
    case S_503_SERVICE_UNAVAILABLE:
      return "Service Unavailable";
    case S_504_GATEWAY_TIMEOUT:
      return "Gateway Timeout";
    case S_505_HTTP_VERSION_NOT_SUPPORTED:
      return "HTTP Version Not Supported";
    case S_507_INSUFFICIENT_STORAGE:
      return "Insufficient Storage";
    case S_509_BANDWIDTH_LIMIT_EXCEEDED:
      return "Bandwidth Limit Exceeded";
    case S_510_NOT_EXTENDED:
      return "Not Extended";
    case S_511_NETWORK_AUTHENTICATION_REQUIRED:
      return "Network Authentication Required";
    default:
      return "";
  }
}

Status intToStatus(int status) {
  switch (status) {
    case 200:
      return S_200_OK;
    case 201:
      return S_201_CREATED;
    case 204:
      return S_204_NO_CONTENT;
    case 206:
      return S_206_PARTIAL_CONTENT;
    case 301:
      return S_301_MOVED_PERMANENTLY;
    case 302:
      return S_302_FOUND;
    case 303:
      return S_303_SEE_OTHER;
    case 307:
      return S_307_TEMPORARY_REDIRECT;
    case 308:
      return S_308_PERMANENT_REDIRECT;
    case 400:
      return S_400_BAD_REQUEST;
    case 401:
      return S_401_UNAUTHORIZED;
    case 402:
      return S_402_PAYMENT_REQUIRED;
    case 403:
      return S_403_FORBIDDEN;
    case 404:
      return S_404_NOT_FOUND;
    case 405:
      return S_405_METHOD_NOT_ALLOWED;
    case 406:
      return S_406_NOT_ACCEPTABLE;
    case 408:
      return S_408_REQUEST_TIMEOUT;
    case 409:
      return S_409_CONFLICT;
    case 410:
      return S_410_GONE;
    case 411:
      return S_411_LENGTH_REQUIRED;
    case 413:
      return S_413_PAYLOAD_TOO_LARGE;
    case 414:
      return S_414_URI_TOO_LONG;
    case 415:
      return S_415_UNSUPPORTED_MEDIA_TYPE;
    case 416:
      return S_416_RANGE_NOT_SATISFIABLE;
    case 417:
      return S_417_EXPECTATION_FAILED;
    case 418:
      return S_418_IM_A_TEAPOT;
    case 426:
      return S_426_UPGRADE_REQUIRED;
    case 428:
      return S_428_PRECONDITION_REQUIRED;
    case 429:
      return S_429_TOO_MANY_REQUESTS;
    case 431:
      return S_431_REQUEST_HEADER_FIELDS_TOO_LARGE;
    case 451:
      return S_451_UNAVAILABLE_FOR_LEGAL_REASONS;
    case 500:
      return S_500_INTERNAL_SERVER_ERROR;
    case 501:
      return S_501_NOT_IMPLEMENTED;
    case 502:
      return S_502_BAD_GATEWAY;
    case 503:
      return S_503_SERVICE_UNAVAILABLE;
    case 504:
      return S_504_GATEWAY_TIMEOUT;
    case 505:
      return S_505_HTTP_VERSION_NOT_SUPPORTED;
    case 507:
      return S_507_INSUFFICIENT_STORAGE;
    case 509:
      return S_509_BANDWIDTH_LIMIT_EXCEEDED;
    case 510:
      return S_510_NOT_EXTENDED;
    case 511:
      return S_511_NETWORK_AUTHENTICATION_REQUIRED;
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

bool isSuccess(Status s) {
  return s >= 200 && s <= 299;
}

bool isRedirect(Status s) {
  return s >= 300 && s <= 399;
}

bool isClientError(Status s) {
  return s >= 400 && s <= 499;
}

bool isServerError(Status s) {
  // Accept all defined 5xx statuses supported by the project (up to 511)
  return s >= 500 && s <= 511;
}

bool isValidStatusCode(int status) {
  try {
    (void)intToStatus(status);
    return true;
  } catch (const std::invalid_argument&) {
    return false;
  }
}

}  // namespace http
