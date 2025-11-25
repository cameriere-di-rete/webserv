#pragma once

#include <string>

class Connection;
class Request;
class Location;

enum HandlerResult { HR_DONE = 0, HR_WOULD_BLOCK = 1, HR_ERROR = -1 };

// Context passed to handlers containing resolved request information
struct HandlerContext {
  const Request* request;
  const Location* location;
  std::string resolved_path;  // Filesystem path for file-based handlers
  bool is_directory;

  HandlerContext()
      : request(NULL), location(NULL), resolved_path(), is_directory(false) {}
};

class IHandler {
 public:
  virtual ~IHandler() {}

  // Check if this handler can process the given request context
  // Returns true if this handler should process the request
  virtual bool canHandle(const HandlerContext& ctx) const = 0;

  // Create a new handler instance configured for the given context
  // Caller takes ownership of returned pointer
  virtual IHandler* create(const HandlerContext& ctx) const = 0;

  virtual HandlerResult start(Connection& conn) = 0;
  virtual HandlerResult resume(Connection& conn) = 0;
};
