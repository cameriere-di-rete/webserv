#include "HeadHandler.hpp"

#include <sstream>

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "Logger.hpp"
#include "Request.hpp"
#include "constants.hpp"
#include "file_utils.hpp"

HeadHandler::HeadHandler() : file_path_() {}

HeadHandler::HeadHandler(const std::string& path) : file_path_(path) {}

HeadHandler::~HeadHandler() {}

bool HeadHandler::canHandle(const HandlerContext& ctx) const {
  // Handle HEAD requests for files (not directories without index)
  if (!ctx.request) {
    return false;
  }
  const std::string& method = ctx.request->request_line.method;
  return method == "HEAD" && !ctx.resolved_path.empty() && !ctx.is_directory;
}

IHandler* HeadHandler::create(const HandlerContext& ctx) const {
  return new HeadHandler(ctx.resolved_path);
}

HandlerResult HeadHandler::start(Connection& conn) {
  LOG(DEBUG) << "HeadHandler: processing HEAD request for fd=" << conn.fd
             << " path=" << file_path_;

  // HEAD is like GET but without the response body
  // Use file_utils to prepare the response headers
  FileInfo fi;
  off_t start = 0, end = 0;

  int fh_res = file_utils::prepareFileResponse(file_path_, NULL, conn.response,
                                               fi, start, end);

  if (fh_res == -1) {
    // File not found - use Connection helper
    conn.prepareErrorResponse(http::S_404_NOT_FOUND);
  } else if (fh_res == -2) {
    // Invalid range (shouldn't happen for HEAD without Range header)
    // prepareFileResponse already filled the response
  } else {
    // Success - file_utils already prepared headers
    // For HEAD, we don't send the body, so close the file
    file_utils::closeFile(fi);
  }

  // HEAD response has headers but no body - clear any body data
  conn.response.getBody().data = "";

  // Serialize only headers (no body)
  std::ostringstream header_stream;
  header_stream << conn.response.startLine() << CRLF;
  header_stream << conn.response.serializeHeaders();
  header_stream << CRLF;
  conn.write_buffer = header_stream.str();

  return HR_DONE;
}

HandlerResult HeadHandler::resume(Connection& conn) {
  // HEAD handler doesn't need resume functionality
  (void)conn;
  return HR_ERROR;
}
