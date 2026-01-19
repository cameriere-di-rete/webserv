#include "Connection.hpp"

#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include "AutoindexHandler.hpp"
#include "CgiHandler.hpp"
#include "ErrorFileHandler.hpp"
#include "FileHandler.hpp"
#include "HttpMethod.hpp"
#include "HttpStatus.hpp"
#include "Location.hpp"
#include "Logger.hpp"
#include "RedirectHandler.hpp"
#include "Server.hpp"
#include "constants.hpp"
#include "file_utils.hpp"
#include "utils.hpp"

Connection::Connection()
    : fd(-1),
      server_fd(-1),
      write_offset(0),
      headers_end_pos(std::string::npos),
      write_ready(false),
      request(),
      response(),
      active_handler(NULL),
      error_pages(),
      read_start(time(NULL)),
      write_start(0) {}

Connection::Connection(int fd)
    : fd(fd),
      server_fd(-1),
      write_offset(0),
      headers_end_pos(std::string::npos),
      write_ready(false),
      request(),
      response(),
      active_handler(NULL),
      error_pages(),
      read_start(time(NULL)),
      write_start(0) {}

Connection::Connection(const Connection& other)
    : fd(other.fd),
      server_fd(other.server_fd),
      remote_addr(other.remote_addr),
      read_buffer(other.read_buffer),
      write_buffer(other.write_buffer),
      write_offset(other.write_offset),
      headers_end_pos(other.headers_end_pos),
      write_ready(other.write_ready),
      request(other.request),
      response(other.response),
      active_handler(NULL),
      error_pages(other.error_pages),
      read_start(other.read_start),
      write_start(other.write_start) {}

Connection::~Connection() {
  clearHandler();
}

Connection& Connection::operator=(const Connection& other) {
  if (this != &other) {
    fd = other.fd;
    server_fd = other.server_fd;
    remote_addr = other.remote_addr;
    read_buffer = other.read_buffer;
    write_buffer = other.write_buffer;
    write_offset = other.write_offset;
    headers_end_pos = other.headers_end_pos;
    write_ready = other.write_ready;
    request = other.request;
    response = other.response;
    clearHandler();
    error_pages = other.error_pages;
    read_start = other.read_start;
    write_start = other.write_start;
  }
  return *this;
}

void Connection::startWritePhase() {
  write_start = time(NULL);
}

bool Connection::isReadTimedOut(int timeout_seconds) const {
  time_t now = time(NULL);
  if (now < read_start) {
    // Clock went backwards, consider not timed out
    return false;
  }
  return (now - read_start) >= timeout_seconds;
}

bool Connection::isWriteTimedOut(int timeout_seconds) const {
  if (write_start == 0) {
    return false;  // Write phase hasn't started yet
  }
  time_t now = time(NULL);
  if (now < write_start) {
    // Clock went backwards, consider not timed out
    return false;
  }
  return (now - write_start) >= timeout_seconds;
}

int Connection::handleRead() {
  while (1) {
    char buf[WRITE_BUF_SIZE] = {0};

    ssize_t r = recv(fd, buf, sizeof(buf), 0);

    if (r < 0) {
      LOG_PERROR(ERROR, "read");
      return -1;
    }

    if (r == 0) {
      LOG(INFO) << "Client disconnected (fd: " << fd << ")";
      return -1;
    }

    // Add new data to persistent buffer
    read_buffer.append(buf, r);

    // Check if the HTTP request headers are complete
    std::size_t pos = read_buffer.find(CRLF CRLF);
    if (pos != std::string::npos) {
      headers_end_pos = pos;
      break;
    }
  }
  return 0;
}

int Connection::handleWrite() {
  while (write_offset < write_buffer.size()) {
    ssize_t w =
        send(fd, write_buffer.c_str() + write_offset,
             static_cast<size_t>(write_buffer.size()) - write_offset, 0);

    LOG(DEBUG) << "Sent " << w << " bytes to fd=" << fd;

    if (w < 0) {
      // Error occurred during send
      LOG_PERROR(ERROR, "write");
      return -1;
    }

    write_offset += static_cast<size_t>(w);
  }

  // If there's an active handler, ask it to resume (streaming, CGI, etc.)
  if (active_handler) {
    HandlerResult hr = active_handler->resume(*this);
    if (hr == HR_WOULD_BLOCK) {
      return 1;
    } else if (hr == HR_ERROR) {
      clearHandler();
      return -1;
    } else {
      // HR_DONE
      clearHandler();
    }
  }

  // All data sent successfully
  return 0;
}

std::string Connection::getHttpVersion() const {
  if (request.request_line.version == "HTTP/1.0" ||
      request.request_line.version == "HTTP/1.1") {
    return request.request_line.version;
  }
  return HTTP_VERSION;
}

void Connection::prepareErrorResponse(http::Status status) {
  response.status_line.version = getHttpVersion();
  response.status_line.status_code = status;
  response.status_line.reason = http::reasonPhrase(status);

  // Check if there's a custom error page configured for this status.
  {
    std::map<http::Status, std::string>::const_iterator it =
        error_pages.find(status);
    if (it != error_pages.end()) {
      // Serve the custom error page using ErrorFileHandler (streams via
      // sendfile() and integrates with the Connection's event loop).
      ErrorFileHandler* efh = new ErrorFileHandler(it->second);
      // Ensure response status reflects the error
      response.status_line.version = getHttpVersion();
      response.status_line.status_code = status;
      response.status_line.reason = http::reasonPhrase(status);
      // Let handler prepare headers and stream body
      HandlerResult hr = efh->start(*this);
      if (hr == HR_WOULD_BLOCK) {
        setHandler(efh);
        return;
      }
      // hr == HR_DONE or HR_ERROR: clean up and fall back to default page
      delete efh;
    }
    LOG(ERROR) << "Failed to open custom error page: " << it->second;
  }

  std::string title = http::statusWithReason(status);
  std::ostringstream body;
  body << "<html>" << CRLF << "<head><title>" << title << "</title></head>"
       << CRLF << "<body>" << CRLF << "<center><h1>" << title
       << "</h1></center>" << CRLF << "</body>" << CRLF << "</html>" << CRLF;

  response.setBodyWithContentType(body.str(), "text/html; charset=utf-8");
  write_buffer = response.serialize();
}

void Connection::setHandler(IHandler* h) {
  clearHandler();
  active_handler = h;
  LOG(DEBUG) << "Connection: setHandler installed handler=" << h
             << " fd=" << fd;
}

void Connection::clearHandler() {
  if (active_handler) {
    LOG(DEBUG) << "Connection: clearHandler deleting handler=" << active_handler
               << " fd=" << fd;
    delete active_handler;
    active_handler = NULL;
  }
}

HandlerResult Connection::executeHandler(IHandler* handler) {
  // setHandler takes ownership of handler and clears any previous handler.
  setHandler(handler);
  IHandler* orig = active_handler;
  HandlerResult hr = orig->start(*this);
  if (hr == HR_WOULD_BLOCK) {
    // Handler will continue later; keep it installed.
    return HR_WOULD_BLOCK;
  } else if (hr == HR_ERROR) {
    // Handler failed; if it's still the active handler, clear it and
    // prepare a 500 error response. If it was replaced during start(),
    // do not touch the currently installed handler (it owns cleanup).
    if (active_handler == orig) {
      clearHandler();
    }
    prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
    return HR_ERROR;
  } else {
    // HR_DONE: handler completed synchronously and response is prepared.
    // Only clear the handler if it's still the original one we invoked.
    if (active_handler == orig) {
      clearHandler();
    }
    return HR_DONE;
  }
}

void Connection::processRequest(const Server& server) {
  LOG(DEBUG) << "Processing request for fd: " << fd;

  // URI is already parsed in Request::parseStartAndHeaders()
  if (!request.uri.isValid()) {
    LOG(INFO) << "Invalid URI: " << request.request_line.uri;
    prepareErrorResponse(http::S_400_BAD_REQUEST);
    return;
  }
  std::string path = request.uri.getPath();

  LOG(DEBUG) << "Request path: " << path;

  Location location = server.matchLocation(path);

  processResponse(location);
}

void Connection::processResponse(const Location& location) {
  LOG(DEBUG) << "Processing response for fd: " << fd;

  // Store error page config (paths already resolved in matchLocation)
  error_pages = location.error_page;

  // Reset response state at the beginning to ensure all handlers start clean
  response = Response();

  // Validate protocol version and allowed method for this location.
  http::Status vstat = validateRequestForLocation(location);
  if (vstat != http::S_0_UNKNOWN) {
    prepareErrorResponse(vstat);
    return;
  }

  // Validate request body (Content-Length/header and stored body) for this
  // location before doing any heavy work.
  http::Status bstat = validateRequestBodyForLocation(location);
  if (bstat != http::S_0_UNKNOWN) {
    prepareErrorResponse(bstat);
    return;
  }

  if (location.redirect_code != http::S_0_UNKNOWN) {
    // Delegate redirect response preparation to a RedirectHandler instance
    RedirectHandler* rh = new RedirectHandler(location);
    HandlerResult hr = executeHandler(rh);
    if (hr == HR_WOULD_BLOCK) {
      return;  // handler will continue later
    }
    // For HR_ERROR and HR_DONE, executeHandler already handled cleanup/error
    // and we should return to finish processing this request.
    return;
  }

  if (!location.cgi_root.empty()) {
    // CGI handling
    std::string resolved_path;
    bool is_directory = false;
    if (!resolvePathForLocation(location, resolved_path, is_directory)) {
      return;  // resolvePathForLocation prepared an error response
    }

    if (is_directory) {
      prepareErrorResponse(http::S_403_FORBIDDEN);
      return;
    }

    IHandler* handler = new CgiHandler(resolved_path, location.cgi_extensions);
    setHandler(handler);

    HandlerResult hr = active_handler->start(*this);
    if (hr == HR_WOULD_BLOCK) {
      return;  // handler will continue later
    } else if (hr == HR_ERROR) {
      clearHandler();
      prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
      return;
    } else {
      // HR_DONE: response prepared in write_buffer
      clearHandler();
      return;
    }
  }

  // Resolve the filesystem path for the request
  std::string resolved_path;
  bool is_directory = false;
  if (!resolvePathForLocation(location, resolved_path, is_directory)) {
    return;  // resolvePathForLocation prepared an error response
  }

  // For POST/PUT/DELETE on directories, use FileHandler which can create files
  std::string method = request.request_line.method;
  if (is_directory &&
      (method == "POST" || method == "PUT")) {
    // FileHandler can handle POST to directory (creates new file)
    IHandler* handler = new FileHandler(resolved_path, request.uri.getPath());
    HandlerResult hr = executeHandler(handler);
    if (hr == HR_WOULD_BLOCK) {
      return;  // handler will continue later
    }
    return;
  }

  // Directory handling (GET/HEAD only)
  if (is_directory) {
    if (location.autoindex) {
      // Delegate to AutoindexHandler (produces directory listing)
      // Pass a user-facing URI path for display in the listing instead of the
      // filesystem path to avoid leaking internal structure.
      std::string display_path = request.uri.getPath();
      if (display_path.empty()) {
        display_path = "/";
      }
      if (display_path[display_path.size() - 1] != '/') {
        display_path += '/';
      }

      AutoindexHandler* ah = new AutoindexHandler(resolved_path, display_path);
      HandlerResult hr = executeHandler(ah);
      if (hr == HR_WOULD_BLOCK) {
        return;  // handler will continue later
      }
      // HR_DONE or HR_ERROR: executeHandler already handled everything
      return;
    } else {
      // Directory listing not allowed
      prepareErrorResponse(http::S_403_FORBIDDEN);
      return;
    }
  }

  // Static file handling - FileHandler handles GET, HEAD, PUT, DELETE
  IHandler* handler = new FileHandler(resolved_path, request.uri.getPath());
  HandlerResult hr = executeHandler(handler);
  if (hr == HR_WOULD_BLOCK) {
    return;  // handler will continue later
  }
}

http::Status Connection::validateRequestForLocation(const Location& location) {
  // 1. Check HTTP protocol version (accept both HTTP/1.0 and HTTP/1.1)
  if (request.request_line.version != "HTTP/1.0" &&
      request.request_line.version != "HTTP/1.1") {
    LOG(INFO) << "Unsupported HTTP version: " << request.request_line.version;
    return http::S_505_HTTP_VERSION_NOT_SUPPORTED;
  }

  // 2. Check HTTP method
  http::Method method;
  try {
    method = http::stringToMethod(request.request_line.method);
  } catch (const std::invalid_argument&) {
    LOG(INFO) << "Not implemented method: " << request.request_line.method;
    return http::S_501_NOT_IMPLEMENTED;
  }

  // Check if method is allowed in this location
  if (location.allow_methods.find(method) == location.allow_methods.end()) {
    LOG(INFO) << "Method not allowed: " << request.request_line.method
              << " for location: " << location.path;
    // Build Allow header with allowed methods
    std::string allow_header;
    for (std::set<http::Method>::const_iterator it =
             location.allow_methods.begin();
         it != location.allow_methods.end(); ++it) {
      if (!allow_header.empty()) {
        allow_header += ", ";
      }
      allow_header += http::methodToString(*it);
    }
    response.addHeader("Allow", allow_header);
    return http::S_405_METHOD_NOT_ALLOWED;
  }
  return http::S_0_UNKNOWN;  // OK
}

http::Status Connection::validateRequestBodyForLocation(
    const Location& location) {
  // Ensure max_request_body was properly inherited/applied
  if (location.max_request_body == kMaxRequestBodyUnset) {
    LOG(ERROR) << "Location max_request_body is unset for location: "
               << location.path;
    return http::S_500_INTERNAL_SERVER_ERROR;
  }

  // If the client provided a Content-Length header, validate it first so we
  // can fail fast before reading/storing potentially large bodies.
  std::string content_length_str;
  if (request.getHeader("Content-Length", content_length_str)) {
    long long content_len = 0;
    if (!safeStrtoll(content_length_str, content_len)) {
      LOG(INFO) << "Malformed Content-Length header: " << content_length_str;
      return http::S_400_BAD_REQUEST;
    }
    if (content_len < 0) {
      LOG(INFO) << "Negative Content-Length header: " << content_length_str;
      return http::S_400_BAD_REQUEST;
    }
    if (static_cast<std::size_t>(content_len) > location.max_request_body) {
      LOG(DEBUG) << "Content-Length " << content_len
                 << " exceeds max_request_body " << location.max_request_body;
      return http::S_413_PAYLOAD_TOO_LARGE;
    }
  }

  // If body has already been read into the request, verify its size too.
  if (request.getBody().size() > location.max_request_body) {
    LOG(DEBUG) << "Request body size " << request.getBody().size()
               << " exceeds max_request_body " << location.max_request_body;
    return http::S_413_PAYLOAD_TOO_LARGE;
  }

  return http::S_0_UNKNOWN;
}

bool Connection::resolvePathForLocation(const Location& location,
                                        std::string& out_path,
                                        bool& out_is_directory) {
  out_is_directory = false;

  // URI is already parsed in Request::parseStartAndHeaders()
  // Validation was done in processRequest(), but check again for safety
  if (!request.uri.isValid()) {
    LOG(INFO) << "Invalid URI: " << request.request_line.uri;
    prepareErrorResponse(http::S_400_BAD_REQUEST);
    return false;
  }

  // Path traversal protection: check for ".." sequences in decoded path
  // The Uri class properly URI-decodes before checking, handling all
  // encoded variants (%2e%2e, %2E%2E, mixed case, etc.)
  if (request.uri.hasPathTraversal()) {
    LOG(INFO) << "Path traversal attempt blocked: " << request.uri.getPath();
    prepareErrorResponse(http::S_403_FORBIDDEN);
    return false;
  }

  // Get the decoded path (query string already stripped by Uri parser)
  std::string uri = request.uri.getDecodedPath();

  // Relative path inside the location
  std::string rel = uri;
  if (!location.path.empty() && location.path != "/") {
    if (rel.find(location.path) == 0) {
      rel = rel.substr(location.path.size());
      if (rel.empty()) {
        rel = "/";
      }
    }
  }

  // Determine the root directory to use:
  // - If cgi_root is set, use it (for CGI scripts)
  // - Otherwise, use the regular root
  std::string root;
  if (!location.cgi_root.empty()) {
    root = location.cgi_root;
  } else if (!location.root.empty()) {
    root = location.root;
  } else {
    prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
    return false;
  }

  std::string path;
  if (!root.empty() && root[root.size() - 1] == '/' && !rel.empty() &&
      rel[0] == '/') {
    path = root + rel.substr(1);
  } else if (!root.empty() && root[root.size() - 1] != '/' && !rel.empty() &&
             rel[0] != '/') {
    path = root + "/" + rel;
  } else {
    path = root + rel;
  }

  struct stat st;
  bool path_is_dir = false;
  bool path_exists = (stat(path.c_str(), &st) == 0);

  if (path_exists && S_ISDIR(st.st_mode)) {
    path_is_dir = true;
    if (!path.empty() && path[path.size() - 1] != '/') {
      path += '/';
    }
  }

  // If path ends with '/' but doesn't exist or isn't a directory, return 404
  if (!path.empty() && path[path.size() - 1] == '/' && !path_is_dir) {
    prepareErrorResponse(http::S_404_NOT_FOUND);
    return false;
  }

  // Try to resolve directory to index file
  if (path_is_dir) {
    bool found_index = false;
    for (std::set<std::string>::const_iterator it = location.index.begin();
         it != location.index.end(); ++it) {
      std::string cand = path + *it;
      if (stat(cand.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
        path = cand;
        found_index = true;
        break;
      }
    }
    if (!found_index) {
      // No index file found - this is a directory request
      out_path = path;
      out_is_directory = true;
      return true;  // Let caller decide what to do with directory
    }
  }

  out_path = path;
  return true;
}

void Connection::logAccess() const {
  // nginx-style combined log format:
  // remote_addr - - [time] "request" status bytes_sent
  // Example: 127.0.0.1 - - [12/Dec/2025:15:45:00 +0000] "GET /index.html
  // HTTP/1.1" 200 1234

  std::string method = request.request_line.method;
  std::string uri = request.request_line.uri;
  std::string version = request.request_line.version;
  int status = static_cast<int>(response.status_line.status_code);
  std::size_t bytes = write_buffer.size();

  // If request wasn't parsed, use placeholders
  if (method.empty()) {
    method = "-";
  }
  if (uri.empty()) {
    uri = "-";
  }
  if (version.empty()) {
    version = "-";
  }

  LOG(INFO) << remote_addr << " \"" << method << " " << uri << " " << version
            << "\" " << status << " " << bytes;
}
