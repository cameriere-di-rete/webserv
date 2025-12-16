#pragma once

#include <string>

#include "IHandler.hpp"
#include "file_utils.hpp"

class Connection;

// FileHandler handles static file operations for GET, HEAD, POST, PUT, DELETE.
// This is a resource-based handler that manages all HTTP methods for static
// files.
class FileHandler : public IHandler {
 public:
  explicit FileHandler(const std::string& path, const std::string& uri = "");
  virtual ~FileHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);

 private:
  // Internal method handlers
  HandlerResult handleGet(Connection& conn);
  HandlerResult handleHead(Connection& conn);
  HandlerResult handlePost(Connection& conn);
  HandlerResult handlePut(Connection& conn);
  HandlerResult handleDelete(Connection& conn);

  // Helper methods for POST/PUT
  bool writeBodyToFile(int fd, const std::string& body, size_t& bytes_written);
  void prepareUploadResponse(Connection& conn, http::Status status,
                             const std::string& resource_path,
                             size_t bytes_written,
                             const std::string* location_uri = NULL);

  std::string path_;
  std::string uri_;
  FileInfo fi_;
  off_t start_offset_;
  off_t end_offset_;
  bool active_;
};
