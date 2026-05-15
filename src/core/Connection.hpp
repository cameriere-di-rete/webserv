#pragma once

#include <sys/types.h>

#include <cstddef>
#include <ctime>
#include <map>
#include <string>

#include "HttpStatus.hpp"
#include "IHandler.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"

class Connection {
 public:
  Connection();
  Connection(int fd);
  Connection(const Connection& other);
  ~Connection();

  Connection& operator=(const Connection& other);

  int fd;
  int server_fd;
  std::string remote_addr;
  std::string read_buffer;
  std::string write_buffer;
  std::size_t write_offset;
  std::size_t headers_end_pos;
  bool write_ready;
  // Cached parsed Content-Length (negative if not present)
  long long parsed_content_length;
  Request request;
  Response response;
  IHandler* active_handler;
  std::map<http::Status, std::string> error_pages;
  time_t read_start;   // Timestamp when connection started (for read timeout)
  time_t write_start;  // Timestamp when write phase started (0 if not started)

  // handleRead returns: -1 = error, 0 = need more data, 1 = ready,
  // 2 = response prepared (error page ready)
  // Now accepts the servers map so it can parse headers that depend on
  // server configuration (location limits) and prepare error responses
  // before returning.
  // `server` is the Server configuration to use for parsing headers.
  // If no server configuration is available, callers should pass a
  // default-constructed `Server` whose defaults indicate unset values.
  int handleRead(const Server& server);
  // Check whether the request body is ready/complete based on headers and
  // any previously parsed Content-Length. Returns `true` when the body is
  // complete (or no body); returns `false` when more data is required.
  // In case of a parsing error this will prepare an error response.
  bool isBodyReady();
  // Parse start line and headers to populate request/URI and determine
  // whether the body should be ignored. Returns: 1 = ready to process response,
  // 0 = wait for more data, 2 = error response prepared.
  int processParsedHeaders(const Server& server);
  void startWritePhase();  // Mark the start of write phase
  bool isReadTimedOut(
      int timeout_seconds) const;  // Check if read phase timed out
  bool isWriteTimedOut(
      int timeout_seconds) const;  // Check if write phase timed out
  int handleWrite();
  void processRequest(const class Server& server);
  void processResponse(const class Location& location);
  void prepareErrorResponse(http::Status status);
  // Get the HTTP version from request, defaulting to HTTP/1.1 if not set
  std::string getHttpVersion() const;
  // Validate request version and method for a given location.
  // Returns http::S_0_UNKNOWN on success, or an http::Status code to send.
  http::Status validateRequestForLocation(const class Location& location);
  // Validate request body size and related headers for a given location.
  // Returns http::S_0_UNKNOWN on success, or an http::Status code to send.
  http::Status validateRequestBodyForLocation(const class Location& location);
  // Resolve the request URI to a filesystem path according to `location`.
  // On success returns true and fills `out_path` and `out_is_directory`.
  // On failure it prepares an error response and returns false.
  bool resolvePathForLocation(const class Location& location,
                              std::string& out_path, bool& out_is_directory);
  void setHandler(IHandler* h);
  void clearHandler();
  // Helper to run a handler's start() and perform common error handling.
  // Returns the HandlerResult from the handler.
  HandlerResult executeHandler(IHandler* handler);
  // Log the completed request/response in nginx-style access log format
  void logAccess() const;
};
