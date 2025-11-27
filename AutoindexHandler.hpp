#pragma once

#include <dirent.h>

#include <string>

#include "IHandler.hpp"

// Simple RAII guard for DIR* to ensure closedir() is called on destruction.
// Placed in the header so it can be reused across translation units.
struct DirGuard {
  DIR* dir;
  explicit DirGuard(DIR* d_) : dir(d_) {}
  ~DirGuard() {
    if (dir) {
      closedir(dir);
    }
  }
  DIR* get() const {
    return dir;
  }
};

class AutoindexHandler : public IHandler {
 public:
  // dirpath: filesystem path to the directory
  // display_path: user-facing URI path to show in the listing (e.g. "/autoindex/")
  explicit AutoindexHandler(const std::string& dirpath,
                            const std::string& display_path);
  virtual ~AutoindexHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);

 private:
  std::string dirpath_;
  std::string uri_path_;
};
