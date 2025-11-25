#pragma once

#include <string>

#include "Connection.hpp"
#include "IHandler.hpp"
#include "file_utils.hpp"

class StaticFileHandler : public IHandler {
 public:
  // Default constructor for prototype registration
  StaticFileHandler();
  explicit StaticFileHandler(const std::string& path);
  virtual ~StaticFileHandler();

  // IHandler interface
  virtual bool canHandle(const HandlerContext& ctx) const;
  virtual IHandler* create(const HandlerContext& ctx) const;
  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);

 private:
  std::string path_;
  FileInfo fi_;
  off_t start_offset_;
  off_t end_offset_;
  bool active_;
};
