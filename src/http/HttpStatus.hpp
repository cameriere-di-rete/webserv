#pragma once

#include <string>

namespace http {

enum Status {
  S_0_UNKNOWN = 0,
  // 2xx Success
  S_200_OK = 200,
  S_201_CREATED = 201,
  S_204_NO_CONTENT = 204,
  S_206_PARTIAL_CONTENT = 206,
  // 3xx Redirection
  S_301_MOVED_PERMANENTLY = 301,
  S_302_FOUND = 302,
  S_303_SEE_OTHER = 303,
  S_307_TEMPORARY_REDIRECT = 307,
  S_308_PERMANENT_REDIRECT = 308,
  // 4xx Client Errors
  S_400_BAD_REQUEST = 400,
  S_401_UNAUTHORIZED = 401,
  S_402_PAYMENT_REQUIRED = 402,
  S_403_FORBIDDEN = 403,
  S_404_NOT_FOUND = 404,
  S_405_METHOD_NOT_ALLOWED = 405,
  S_406_NOT_ACCEPTABLE = 406,
  S_408_REQUEST_TIMEOUT = 408,
  S_409_CONFLICT = 409,
  S_410_GONE = 410,
  S_411_LENGTH_REQUIRED = 411,
  S_413_PAYLOAD_TOO_LARGE = 413,
  S_414_URI_TOO_LONG = 414,
  S_415_UNSUPPORTED_MEDIA_TYPE = 415,
  S_416_RANGE_NOT_SATISFIABLE = 416,
  S_417_EXPECTATION_FAILED = 417,
  S_418_IM_A_TEAPOT = 418,
  S_426_UPGRADE_REQUIRED = 426,
  S_428_PRECONDITION_REQUIRED = 428,
  S_429_TOO_MANY_REQUESTS = 429,
  S_431_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
  S_451_UNAVAILABLE_FOR_LEGAL_REASONS = 451,
  // 5xx Server Errors
  S_500_INTERNAL_SERVER_ERROR = 500,
  S_501_NOT_IMPLEMENTED = 501,
  S_502_BAD_GATEWAY = 502,
  S_503_SERVICE_UNAVAILABLE = 503,
  S_504_GATEWAY_TIMEOUT = 504,
  S_505_HTTP_VERSION_NOT_SUPPORTED = 505,
  S_507_INSUFFICIENT_STORAGE = 507,
  S_509_BANDWIDTH_LIMIT_EXCEEDED = 509,
  S_510_NOT_EXTENDED = 510,
  S_511_NETWORK_AUTHENTICATION_REQUIRED = 511
};

// Convert int to Status enum; throws std::invalid_argument on unknown code
Status intToStatus(int status);

std::string reasonPhrase(Status s);

// Return a single string containing the numeric status and reason phrase,
// e.g. "404 Not Found". Accept only the enum to avoid casts.
std::string statusWithReason(Status s);

// Classification helpers
bool isSuccess(Status status);
bool isRedirect(Status status);
bool isClientError(Status status);
bool isServerError(Status status);

// Check if status code (int) is within valid HTTP status code range
bool isValidStatusCode(int status);

}  // namespace http
