#pragma once

#include <dirent.h>

#include <string>

#include "IHandler.hpp"

class AutoindexHandler : public IHandler {
 public:
  // dirpath: filesystem path to the directory
  // display_path: user-facing URI path to show in the listing (e.g.
  // "/autoindex/")
  explicit AutoindexHandler(const std::string& dirpath,
                            const std::string& display_path);
  virtual ~AutoindexHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);

 private:
  std::string dirpath_;
  std::string uri_path_;
};
