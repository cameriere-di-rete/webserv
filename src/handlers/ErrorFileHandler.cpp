#include "ErrorFileHandler.hpp"

#include <unistd.h>

#include "Connection.hpp"
#include "Logger.hpp"
#include "constants.hpp"

ErrorFileHandler::ErrorFileHandler(const std::string& path)
    : path_(path), fi_(), offset_(0), end_offset_(-1), active_(false) {
  fi_.fd = -1;
}

ErrorFileHandler::~ErrorFileHandler() {
  if (fi_.fd >= 0) {
    file_utils::closeFile(fi_);
  }
}

HandlerResult ErrorFileHandler::start(Connection& conn) {
  // Probe the file to get size and content type without keeping it open.
  FileInfo probe;
  if (!file_utils::openFile(path_, probe)) {
    LOG(ERROR) << "ErrorFileHandler: openFile failed for " << path_;
    return HR_ERROR;
  }
  // Store metadata; close the probe fd so we don't hold it across event loop
  fi_.fd = -1;
  fi_.size = probe.size;
  fi_.content_type = probe.content_type;
  file_utils::closeFile(probe);

  offset_ = 0;
  end_offset_ = fi_.size - 1;
  active_ = true;

  // Prepare headers with error status already set by Connection
  conn.response.addHeader("Content-Type", fi_.content_type);
  {
    std::ostringstream tmp;
    tmp << fi_.size;
    conn.response.addHeader("Content-Length", tmp.str());
  }
  std::ostringstream header_stream;
  header_stream << conn.response.startLine() << CRLF;
  header_stream << conn.response.serializeHeaders();
  header_stream << CRLF;
  conn.write_buffer = header_stream.str();
  conn.write_offset = 0;

  return HR_WOULD_BLOCK;  // body will be streamed in resume()
}

HandlerResult ErrorFileHandler::resume(Connection& conn) {
  if (!active_) {
    return HR_DONE;
  }

  // Ensure file is opened for streaming
  if (fi_.fd < 0) {
    if (!file_utils::openFile(path_, fi_)) {
      LOG(ERROR) << "ErrorFileHandler: openFile failed in resume for " << path_;
      return HR_ERROR;
    }
  }
  LOG(DEBUG) << "ErrorFileHandler: streaming " << path_ << " fd=" << fi_.fd
             << " offset=" << offset_ << " end=" << end_offset_;

  int r = file_utils::streamToSocket(conn.fd, fi_.fd, offset_, end_offset_ + 1);
  LOG(DEBUG) << "ErrorFileHandler: streamToSocket returned " << r
             << " new offset=" << offset_;
  if (r < 0) {
    file_utils::closeFile(fi_);
    active_ = false;
    return HR_ERROR;
  }
  if (r == 1) {
    return HR_WOULD_BLOCK;
  }
  file_utils::closeFile(fi_);
  active_ = false;
  return HR_DONE;
}

int ErrorFileHandler::getMonitorFd() const {
  return -1;
}
