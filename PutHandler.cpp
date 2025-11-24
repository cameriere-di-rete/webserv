#include "PutHandler.hpp"

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "Logger.hpp"
#include "constants.hpp"

PutHandler::PutHandler() {}

PutHandler::~PutHandler() {}

HandlerResult PutHandler::start(Connection& conn) {
  LOG(DEBUG) << "PutHandler: processing PUT request for fd=" << conn.fd;

  std::string uri = conn.request.request_line.uri;

  // Basic path traversal protection
  if (uri.find("..") != std::string::npos) {
    LOG(INFO) << "PutHandler: Path traversal attempt: " << uri;
    conn.prepareErrorResponse(http::S_403_FORBIDDEN);
    return HR_DONE;
  }

  // Simple PUT implementation: create/update resource
  conn.response.status_line.version = HTTP_VERSION;
  conn.response.status_line.status_code = http::S_200_OK;
  conn.response.status_line.reason = http::reasonPhrase(http::S_200_OK);

  std::ostringstream body;
  body << "PUT request processed successfully" << CRLF;
  body << "Resource: " << uri << CRLF;
  body << "Content received: " << conn.request.getBody().size() << " bytes"
       << CRLF;

  conn.response.getBody().data = body.str();
  conn.response.addHeader("Content-Type", "text/plain; charset=utf-8");

  std::ostringstream len;
  len << conn.response.getBody().size();
  conn.response.addHeader("Content-Length", len.str());

  conn.write_buffer = conn.response.serialize();

  return HR_DONE;
}

HandlerResult PutHandler::resume(Connection& conn) {
  // PUT handler doesn't need resume functionality for now
  (void)conn;
  return HR_ERROR;
}
