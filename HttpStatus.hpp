#pragma once

#include <string>

namespace http {

enum Status {
  OK = 200,
  CREATED = 201,
  NO_CONTENT = 204,
  MOVED_PERMANENTLY = 301,
  FOUND = 302,
  SEE_OTHER = 303,
  TEMPORARY_REDIRECT = 307,
  PERMANENT_REDIRECT = 308,
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  METHOD_NOT_ALLOWED = 405,
  PAYLOAD_TOO_LARGE = 413,
  URI_TOO_LONG = 414,
  INTERNAL_SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501,
  BAD_GATEWAY = 502,
  SERVICE_UNAVAILABLE = 503,
  GATEWAY_TIMEOUT = 504
};

// Convert int to Status enum; throws std::invalid_argument on unknown code
Status intToStatus(int status);

std::string reasonPhrase(Status s);
std::string statusToString(Status s);

// Return a single string containing the numeric status and reason phrase,
// e.g. "404 Not Found". Accept only the enum to avoid casts.
std::string statusWithReason(Status s);

// Classification helpers
bool isSuccess(int status);
bool isRedirect(int status);
bool isClientError(int status);
bool isServerError(int status);

// Check if status is within valid HTTP status code range
bool isValidStatusCode(int status);

} // namespace http
