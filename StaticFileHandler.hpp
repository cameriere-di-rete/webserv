#pragma once

#include <string>

#include "Connection.hpp"
#include "IHandler.hpp"
#include "file_utils.hpp"

class StaticFileHandler : public IHandler {
 public:
  explicit StaticFileHandler(const std::string& path);
  virtual ~StaticFileHandler();

  virtual HandlerResult start(Connection& conn, const Location& loc);
  virtual HandlerResult resume(Connection& conn);

 private:
  std::string path_;
  FileInfo fi_;
  off_t start_offset_;
  off_t end_offset_;
  bool active_;
};
