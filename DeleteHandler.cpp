#include "DeleteHandler.hpp"

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "Logger.hpp"
#include "constants.hpp"

DeleteHandler::DeleteHandler() {}

DeleteHandler::~DeleteHandler() {}

HandlerResult DeleteHandler::start(Connection& conn) {
  LOG(DEBUG) << "DeleteHandler: processing DELETE request for fd=" << conn.fd;

  std::string uri = conn.request.request_line.uri;

  // Basic path traversal protection
  if (uri.find("..") != std::string::npos) {
    LOG(INFO) << "DeleteHandler: Path traversal attempt: " << uri;
    conn.prepareErrorResponse(http::S_403_FORBIDDEN);
    return HR_DONE;
  }

  // Simple DELETE implementation
  conn.response.status_line.version = HTTP_VERSION;
  conn.response.status_line.status_code = http::S_204_NO_CONTENT;
  conn.response.status_line.reason = http::reasonPhrase(http::S_204_NO_CONTENT);

  // 204 No Content should not have a body
  conn.response.getBody().data = "";
  conn.response.addHeader("Content-Length", "0");

  conn.write_buffer = conn.response.serialize();

  LOG(INFO) << "DeleteHandler: Resource " << uri << " marked for deletion";

  return HR_DONE;
}

HandlerResult DeleteHandler::resume(Connection& conn) {
  // DELETE handler doesn't need resume functionality for now
  (void)conn;
  return HR_ERROR;
}
