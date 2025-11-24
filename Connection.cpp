#include "Connection.hpp"

#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "Body.hpp"
#include "EchoHandler.hpp"
#include "HttpMethod.hpp"
#include "HttpStatus.hpp"
#include "Location.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "StaticFileHandler.hpp"
#include "constants.hpp"

Connection::Connection()
    : fd(-1),
      server_fd(-1),
      write_offset(0),
      headers_end_pos(std::string::npos),
      write_ready(false),
      request(),
      response(),
      active_handler(NULL) {}

Connection::Connection(int fd)
    : fd(fd),
      server_fd(-1),
      write_offset(0),
      headers_end_pos(std::string::npos),
      write_ready(false),
      request(),
      response(),
      active_handler(NULL) {}

Connection::Connection(const Connection& other)
    : fd(other.fd),
      server_fd(other.server_fd),
      read_buffer(other.read_buffer),
      write_buffer(other.write_buffer),
      write_offset(other.write_offset),
      headers_end_pos(other.headers_end_pos),
      write_ready(other.write_ready),
      request(other.request),
      response(other.response),
      active_handler(NULL) {}

Connection::~Connection() {
  clearHandler();
}

Connection& Connection::operator=(const Connection& other) {
  if (this != &other) {
    fd = other.fd;
    server_fd = other.server_fd;
    read_buffer = other.read_buffer;
    write_buffer = other.write_buffer;
    write_offset = other.write_offset;
    headers_end_pos = other.headers_end_pos;
    write_ready = other.write_ready;
    request = other.request;
    response = other.response;
    clearHandler();
  }
  return *this;
}

int Connection::handleRead() {
  while (1) {
    char buf[WRITE_BUF_SIZE] = {0};

    ssize_t r = recv(fd, buf, sizeof(buf), 0);

    if (r < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;
      }
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
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;
      }
      // Error occurred
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

void Connection::prepareErrorResponse(http::Status status) {
  response.status_line.version = HTTP_VERSION;
  response.status_line.status_code = status;
  response.status_line.reason = http::reasonPhrase(status);

  std::string title = http::statusWithReason(status);
  std::ostringstream body;
  body << "<html>" << CRLF << "<head><title>" << title << "</title></head>"
       << CRLF << "<body>" << CRLF << "<center><h1>" << title
       << "</h1></center>" << CRLF << "</body>" << CRLF << "</html>" << CRLF;

  response.getBody().data = body.str();
  response.addHeader("Content-Type", "text/html; charset=utf-8");
  std::ostringstream oss;
  oss << response.getBody().size();
  response.addHeader("Content-Length", oss.str());
  write_buffer = response.serialize();
}

void Connection::setHandler(IHandler* h) {
  clearHandler();
  active_handler = h;
}

void Connection::clearHandler() {
  if (active_handler) {
    delete active_handler;
    active_handler = NULL;
  }
}

void Connection::processRequest(const Server& server) {
  LOG(DEBUG) << "Processing request for fd: " << fd;

  // 1. Parse request headers (already done in ServerManager)
  // 2. Get pathname from URI
  std::string path = request.request_line.uri;

  // Extract path from URI (remove query string if present)
  std::size_t query_pos = path.find('?');
  if (query_pos != std::string::npos) {
    path = path.substr(0, query_pos);
  }

  LOG(DEBUG) << "Request path: " << path;

  // 3. Match URI with Server.Location
  Location location = server.matchLocation(path);

  // 4. Process response based on location
  processResponse(location);
}

void Connection::processResponse(const Location& location) {
  LOG(DEBUG) << "Processing response for fd: " << fd;

  // 1. Check HTTP protocol version
  if (request.request_line.version != HTTP_VERSION) {
    LOG(INFO) << "Unsupported HTTP version: " << request.request_line.version;
    prepareErrorResponse(http::S_505_HTTP_VERSION_NOT_SUPPORTED);
    return;
  }

  // 2. Check HTTP method
  http::Method method;
  try {
    method = http::stringToMethod(request.request_line.method);
  } catch (const std::invalid_argument&) {
    // Method not in implemented_methods
    LOG(INFO) << "Not implemented method: " << request.request_line.method;
    prepareErrorResponse(http::S_501_NOT_IMPLEMENTED);
    return;
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
    prepareErrorResponse(http::S_405_METHOD_NOT_ALLOWED);
    return;
  }

  if (location.cgi) {
    // handleCGI();
    return;
  }

  if (location.redirect_code != http::S_0_UNKNOWN) {
    // handleRedirect();
    return;
  }

  // handleFile
  // handleIndexFile

  /* prepare response: if GET -> try to stream file, otherwise echo the
         request body as before */
  // reset response state
  response = Response();

  const std::string& req_method = request.request_line.method;
  const std::string& uri = request.request_line.uri;

  if (req_method == "GET") {
    // map URI to filesystem path using Location configuration
    std::string rel = uri;
    // If location.path is a prefix of the URI, strip it
    if (!location.path.empty() && location.path != "/") {
      if (rel.find(location.path) == 0) {
        rel = rel.substr(location.path.size());
        if (rel.empty()) {
          rel = "/";
        }
      }
    }

    // Build filesystem path from location root
    if (location.root.empty()) {
      // misconfigured location: no root specified
      prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
      return;
    }
    std::string root = location.root;
    // Normalize separators when concatenating
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

    // If the path is a directory (or ends with '/'), try index files from the
    // location
    struct stat st;
    bool path_is_dir = false;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
      path_is_dir = true;
      if (path[path.size() - 1] != '/') {
        path += '/';
      }
    }
    if (path_is_dir || (!path.empty() && path[path.size() - 1] == '/')) {
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
        if (location.autoindex) {
          // autoindex not implemented yet
          prepareErrorResponse(http::S_501_NOT_IMPLEMENTED);
        } else {
          prepareErrorResponse(http::S_404_NOT_FOUND);
        }
        return;
      }
    }

    // basic path traversal protection
    if (path.find("..") != std::string::npos) {
      prepareErrorResponse(http::S_403_FORBIDDEN);
      return;
    }

    // create a StaticFileHandler to serve the file
    StaticFileHandler* h = new StaticFileHandler(path);
    setHandler(h);
    HandlerResult hr = active_handler->start(*this, location);
    if (hr == HR_WOULD_BLOCK) {
      // handler will stream the body later via resume()
      return;
    } else if (hr == HR_ERROR) {
      clearHandler();
      prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
      return;
    } else {
      // HR_DONE: response already prepared in this->write_buffer
      clearHandler();
      // fall through to allow caller to write response
    }
  } else {
    // non-GET: delegate to EchoHandler
    EchoHandler* h = new EchoHandler();
    setHandler(h);
    HandlerResult hr = active_handler->start(*this, location);
    if (hr == HR_WOULD_BLOCK) {
      return;  // handler will complete later
    } else if (hr == HR_ERROR) {
      clearHandler();
      prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
      return;
    } else {
      // HR_DONE: response prepared in write_buffer
      clearHandler();
      // fall through to allow caller to write response
    }
  }

  if (location.autoindex) {
    // handleAutoindex();
    return;
  }
}
