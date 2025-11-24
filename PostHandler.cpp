#include "PostHandler.hpp"

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "Logger.hpp"
#include "constants.hpp"

PostHandler::PostHandler() {}

PostHandler::~PostHandler() {}

HandlerResult PostHandler::start(Connection& conn) {
  LOG(DEBUG) << "PostHandler: processing POST request for fd=" << conn.fd;

  // For now, implement a simple POST handler that saves to a file
  // In a real implementation, this might save to a database, process form data,
  // etc.

  std::string uri = conn.request.request_line.uri;
  std::string upload_path = "./www/uploads" + uri;

  // Basic path traversal protection
  if (upload_path.find("..") != std::string::npos) {
    LOG(INFO) << "PostHandler: Path traversal attempt: " << upload_path;
    conn.prepareErrorResponse(http::S_403_FORBIDDEN);
    return HR_DONE;
  }

  // Simple implementation: echo back the POST data with success message
  conn.response.status_line.version = HTTP_VERSION;
  conn.response.status_line.status_code = http::S_201_CREATED;
  conn.response.status_line.reason = http::reasonPhrase(http::S_201_CREATED);

  std::ostringstream body;
  body << "POST request processed successfully" << CRLF;
  body << "URI: " << uri << CRLF;
  body << "Content received: " << conn.request.getBody().size() << " bytes"
       << CRLF;
  body << "Data:" << CRLF << conn.request.getBody().data;

  conn.response.getBody().data = body.str();
  conn.response.addHeader("Content-Type", "text/plain; charset=utf-8");

  std::ostringstream len;
  len << conn.response.getBody().size();
  conn.response.addHeader("Content-Length", len.str());

  conn.write_buffer = conn.response.serialize();

  return HR_DONE;
}

HandlerResult PostHandler::resume(Connection& conn) {
  // POST handler doesn't need resume functionality for now
  (void)conn;
  return HR_ERROR;
}
