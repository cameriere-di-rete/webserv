#include "FileHandler.hpp"
#include "Response.hpp"
#include "constants.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

namespace FileHandler {

std::string guessMime(const std::string &path) {
  std::string def = "application/octet-stream";
  std::size_t pos = path.rfind('.');
  if (pos == std::string::npos)
    return def;
  std::string ext = path.substr(pos + 1);
  if (ext == "html" || ext == "htm")
    return "text/html; charset=utf-8";
  if (ext == "txt")
    return "text/plain; charset=utf-8";
  if (ext == "css")
    return "text/css";
  if (ext == "js")
    return "application/javascript";
  if (ext == "jpg" || ext == "jpeg")
    return "image/jpeg";
  if (ext == "png")
    return "image/png";
  if (ext == "gif")
    return "image/gif";
  return def;
}

bool openFile(const std::string &path, FileInfo &out) {
  out.fd = -1;
  out.size = 0;
  out.content_type.clear();

  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    std::cerr << "FileHandler: openFile failed for '" << path
              << "' : " << strerror(errno) << "\n";
    return false;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    close(fd);
    return false;
  }

  out.fd = fd;
  out.size = st.st_size;
  out.content_type = guessMime(path);
  std::cout << "FileHandler: opened '" << path << "' fd=" << out.fd
            << " size=" << out.size << " type=" << out.content_type << "\n";
  return true;
}

void closeFile(FileInfo &fi) {
  if (fi.fd >= 0) {
    std::cout << "FileHandler: closing fd=" << fi.fd << "\n";
    close(fi.fd);
    fi.fd = -1;
  }
  fi.size = 0;
  fi.content_type.clear();
}

int streamToSocket(int sock_fd, int file_fd, off_t &offset, off_t max_offset) {
  if (offset >= max_offset)
    return 0; // nothing to send

  std::cout << "FileHandler: streamToSocket fd=" << file_fd
            << " to sock=" << sock_fd << " offset=" << offset
            << " max=" << max_offset << "\n";
  while (offset < max_offset) {
    size_t to_send = static_cast<size_t>(max_offset - offset);
    if (to_send > WRITE_BUF_SIZE)
      to_send = WRITE_BUF_SIZE;

    ssize_t s = sendfile(sock_fd, file_fd, &offset, to_send);
    if (s < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        std::cout << "FileHandler: sendfile would block (EAGAIN)\n";
        return 1;
      }
      std::cerr << "FileHandler: sendfile error: " << strerror(errno) << "\n";
      return -1;
    }

    if (s == 0) {
      std::cout << "FileHandler: sendfile returned 0 (EOF?)\n";
      break; // no more data
    }

    std::cout << "FileHandler: sendfile wrote " << s
              << " bytes, new offset=" << offset << "\n";
  }

  return (offset >= max_offset) ? 0 : 1;
}

bool parseRange(const std::string &rangeHeader, off_t file_size,
                off_t &out_start, off_t &out_end) {
  // expected to start with "bytes="
  const std::string prefix = "bytes=";
  if (rangeHeader.compare(0, prefix.size(), prefix) != 0)
    return false;

  std::string spec = rangeHeader.substr(prefix.size());
  std::size_t dash = spec.find('-');
  if (dash == std::string::npos)
    return false;

  std::string first = spec.substr(0, dash);
  std::string second = spec.substr(dash + 1);

  if (first.empty()) {
    // suffix length: -N -> last N bytes
    if (second.empty())
      return false;
    long suffix = atol(second.c_str());
    if (suffix <= 0)
      return false;
    if (suffix > file_size)
      suffix = file_size;
    out_start = file_size - suffix;
    out_end = file_size - 1;
    return true;
  }

  // first is present
  off_t start = atoll(first.c_str());
  off_t end = -1;
  if (second.empty()) {
    end = file_size - 1;
  } else {
    end = atoll(second.c_str());
  }

  if (start < 0 || start >= file_size)
    return false;
  if (end < start)
    return false;
  if (end >= file_size)
    end = file_size - 1;

  out_start = start;
  out_end = end;
  return true;
}

int prepareFileResponse(const std::string &path, const std::string *rangeHeader,
                        Response &outResponse, FileInfo &outFile,
                        off_t &out_start, off_t &out_end) {
  outFile.fd = -1;
  outFile.size = 0;
  outFile.content_type.clear();

  if (!openFile(path, outFile)) {
    std::cout
        << "FileHandler: prepareFileResponse - openFile returned false for '"
        << path << "'\n";
    return -1; // not found
  }

  off_t file_size = outFile.size;
  bool is_partial = false;

  if (rangeHeader) {
    off_t s = 0, e = 0;
    if (!parseRange(*rangeHeader, file_size, s, e)) {
      // invalid range -> 416
      std::cout << "FileHandler: prepareFileResponse - invalid range '"
                << *rangeHeader << "' for file=" << path
                << " size=" << file_size << "\n";
      outResponse.status_line.version = HTTP_VERSION;
      outResponse.status_line.status_code = 416;
      outResponse.status_line.reason = "Range Not Satisfiable";
      std::ostringstream body;
      body << "416 Range Not Satisfiable";
      outResponse.getBody().data = body.str();
      {
        std::ostringstream tmp;
        tmp << outResponse.getBody().size();
        outResponse.addHeader("Content-Length", tmp.str());
      }
      // indicate actual size
      std::ostringstream cr;
      cr << "bytes */" << file_size;
      outResponse.addHeader("Content-Range", cr.str());
      // close file
      closeFile(outFile);
      return -2;
    }
    std::cout << "FileHandler: prepareFileResponse - parsed range start=" << s
              << " end=" << e << "\n";
    out_start = s;
    out_end = e;
    is_partial = true;
  } else {
    out_start = 0;
    out_end = file_size - 1;
  }

  if (is_partial) {
    outResponse.status_line.version = HTTP_VERSION;
    outResponse.status_line.status_code = 206;
    outResponse.status_line.reason = "Partial Content";
    off_t len = out_end - out_start + 1;
    {
      std::ostringstream tmp;
      tmp << len;
      outResponse.addHeader("Content-Length", tmp.str());
    }
    std::ostringstream cr;
    cr << "bytes " << out_start << "-" << out_end << "/" << file_size;
    outResponse.addHeader("Content-Range", cr.str());
  } else {
    outResponse.status_line.version = HTTP_VERSION;
    outResponse.status_line.status_code = 200;
    outResponse.status_line.reason = "OK";
    {
      std::ostringstream tmp;
      tmp << file_size;
      outResponse.addHeader("Content-Length", tmp.str());
    }
  }

  outResponse.addHeader("Content-Type", outFile.content_type);
  std::cout << "FileHandler: prepareFileResponse prepared response code="
            << outResponse.status_line.status_code
            << " content-type=" << outFile.content_type
            << " length=" << outFile.size << "\n";
  return 0;
}

} // namespace FileHandler
