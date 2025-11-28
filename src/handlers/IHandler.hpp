#pragma once

class Connection;

enum HandlerResult { HR_DONE = 0, HR_WOULD_BLOCK = 1, HR_ERROR = -1 };

// Base interface for all request handlers.
// Handlers are organized by resource type, not HTTP method.
// Each handler manages a specific type of resource and handles
// all applicable HTTP methods for that resource internally.
class IHandler {
 public:
  virtual ~IHandler() {}

  // Start processing the request. Called once when handler is first invoked.
  // Returns HR_DONE if complete, HR_WOULD_BLOCK if needs to continue later,
  // HR_ERROR on failure.
  virtual HandlerResult start(Connection& conn) = 0;

  // Continue processing after I/O is ready (for streaming, CGI, etc.)
  // Returns HR_DONE when complete, HR_WOULD_BLOCK if more work needed,
  // HR_ERROR on failure.
  virtual HandlerResult resume(Connection& conn) = 0;

  // Returns the file descriptor to monitor for I/O readiness (e.g., CGI pipe).
  // Returns -1 if no additional FD needs monitoring.
  // This allows epoll to monitor handler-specific FDs for non-blocking I/O.
  virtual int getMonitorFd() const
  {
    return -1;
  }
};
