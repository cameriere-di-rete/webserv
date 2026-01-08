#pragma once

#include "IHandler.hpp"
#include "file_utils.hpp"

class Connection;

class ErrorFileHandler : public IHandler {
 public:
  explicit ErrorFileHandler(const std::string& path);
  virtual ~ErrorFileHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);
  virtual int getMonitorFd() const;

 private:
  std::string path_;
  FileInfo fi_;
  off_t offset_;
  off_t end_offset_;
  bool active_;
};
