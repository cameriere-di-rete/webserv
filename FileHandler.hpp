#pragma once

#include <string>

#include "IHandler.hpp"
#include "file_utils.hpp"

class Connection;

// FileHandler handles static file operations for GET, HEAD, PUT, and DELETE.
// This is a resource-based handler that manages all HTTP methods for static
// files.
class FileHandler : public IHandler {
 public:
  explicit FileHandler(const std::string& path);
  virtual ~FileHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);

 private:
  // Internal method handlers
  HandlerResult handleGet(Connection& conn);
  HandlerResult handleHead(Connection& conn);
  HandlerResult handlePut(Connection& conn);
  HandlerResult handleDelete(Connection& conn);

  std::string path_;
  FileInfo fi_;
  off_t start_offset_;
  off_t end_offset_;
  bool active_;
};
