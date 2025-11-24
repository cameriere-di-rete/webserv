#include "StaticFileHandler.hpp"

#include <cstring>
#include <sstream>

#include "Request.hpp"
#include "constants.hpp"
#include "file_utils.hpp"

StaticFileHandler::StaticFileHandler(const std::string& path)
    : path_(path), fi_(), start_offset_(0), end_offset_(-1), active_(false) {}

StaticFileHandler::~StaticFileHandler() {
  // ensure file closed if still open
  if (fi_.fd >= 0) {
    file_utils::closeFile(fi_);
  }
}

HandlerResult StaticFileHandler::start(Connection& conn) {
  std::string range;
  const std::string* rangePtr = NULL;
  if (conn.request.getHeader("Range", range)) {
    rangePtr = &range;
  }

  off_t out_start = 0, out_end = 0;
  int r = file_utils::prepareFileResponse(path_, rangePtr, conn.response, fi_,
                                          out_start, out_end);
  if (r == -1) {
    // file not found -> prepare a 404 using Connection helper
    conn.prepareErrorResponse(http::S_404_NOT_FOUND);
    return HR_DONE;
  }
  if (r == -2) {
    // invalid range: prepareFileResponse already filled response
    return HR_DONE;
  }

  // success: prepareFileResponse filled headers and fi_
  start_offset_ = out_start;
  end_offset_ = out_end;
  active_ = true;

  // write only headers to connection so we can stream body
  std::ostringstream header_stream;
  header_stream << conn.response.startLine() << CRLF;
  header_stream << conn.response.serializeHeaders();
  header_stream << CRLF;
  conn.write_buffer = header_stream.str();
  conn.write_offset = 0;

  return HR_WOULD_BLOCK;  // body streaming will occur via resume/sendfile
}

HandlerResult StaticFileHandler::resume(Connection& conn) {
  if (!active_) {
    return HR_DONE;
  }
  int r = file_utils::streamToSocket(conn.fd, fi_.fd, start_offset_,
                                     end_offset_ + 1);
  if (r < 0) {
    // error
    file_utils::closeFile(fi_);
    active_ = false;
    return HR_ERROR;
  }
  if (r == 1) {
    // would block
    return HR_WOULD_BLOCK;
  }
  // finished
  file_utils::closeFile(fi_);
  active_ = false;
  return HR_DONE;
}
