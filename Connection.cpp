#include "Connection.hpp"

#include <sys/socket.h>

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "Body.hpp"
#include "HttpMethod.hpp"
#include "HttpStatus.hpp"
#include "Location.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "constants.hpp"

Connection::Connection()
    : fd(-1),
      server_fd(-1),
      write_offset(0),
      headers_end_pos(std::string::npos),
      write_ready(false) {}

Connection::Connection(int file_descriptor)
    : fd(file_descriptor),
      server_fd(-1),
      write_offset(0),
      headers_end_pos(std::string::npos),
      write_ready(false) {}

Connection::Connection(const Connection& other)
    : fd(other.fd),
      server_fd(other.server_fd),
      read_buffer(other.read_buffer),
      write_buffer(other.write_buffer),
      write_offset(other.write_offset),
      headers_end_pos(other.headers_end_pos),
      write_ready(other.write_ready),
      request(other.request),
      response(other.response) {}

Connection::~Connection() {}

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
  }
  return *this;
}

int Connection::handleRead() {
  while (true) {
    char buf[WRITE_BUF_SIZE] = {0};

    ssize_t bytes_read = recv(fd, buf, sizeof(buf), 0);

    if (bytes_read < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;
      }
      LOG_PERROR(ERROR, "read");
      return -1;
    }

    if (bytes_read == 0) {
      LOG(INFO) << "Client disconnected (fd: " << fd << ")";
      return -1;
    }

    // Add new data to persistent buffer
    read_buffer.append(buf, static_cast<std::size_t>(bytes_read));

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
    ssize_t bytes_written =
        send(fd, write_buffer.c_str() + write_offset,
             static_cast<size_t>(write_buffer.size()) - write_offset, 0);

    LOG(DEBUG) << "Sent " << bytes_written << " bytes to fd=" << fd;

    if (bytes_written < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;
      }
      // Error occurred
      LOG_PERROR(ERROR, "write");
      return -1;
    }

    write_offset += static_cast<size_t>(bytes_written);
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
  http::Method method = http::GET;
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

  // For now, just return a simple 200 OK response
  // TODO: Implement redirect, CGI, file and directory handling
  LOG(DEBUG) << "Request validation passed, sending 200 OK";
  response.status_line.version = HTTP_VERSION;
  response.status_line.status_code = http::S_200_OK;
  response.status_line.reason = http::reasonPhrase(http::S_200_OK);
  response.setBody(Body(read_buffer));
  std::ostringstream oss2;
  oss2 << response.getBody().size();
  response.addHeader("Content-Length", oss2.str());
  response.addHeader("Content-Type", "text/plain; charset=utf-8");
  write_buffer = response.serialize();
}
