#include "PostUploadHandler.hpp"

#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "Logger.hpp"
#include "constants.hpp"

PostUploadHandler::PostUploadHandler() {}

PostUploadHandler::~PostUploadHandler() {}

HandlerResult PostUploadHandler::start(Connection& conn) {
  LOG(DEBUG) << "PostUploadHandler: processing POST request for fd=" << conn.fd;

  std::string uri = conn.request.request_line.uri;
  std::string upload_path = "./www/uploads" + uri;

  // Basic path traversal protection
  if (upload_path.find("..") != std::string::npos) {
    LOG(INFO) << "PostUploadHandler: Path traversal attempt: " << upload_path;
    conn.prepareErrorResponse(http::S_403_FORBIDDEN);
    return HR_DONE;
  }

  // Create uploads directory if it doesn't exist
  struct stat st;
  if (stat("./www/uploads", &st) != 0) {
    if (mkdir("./www/uploads", 0755) != 0) {
      LOG(ERROR) << "PostUploadHandler: Failed to create uploads directory";
      conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
      return HR_DONE;
    }
  }

  // Create subdirectories if needed (recursive creation)
  if (upload_path.find("/") != std::string::npos) {
    size_t last_slash = upload_path.find_last_of("/");
    std::string dir_path = upload_path.substr(0, last_slash);
    
    // Create directories recursively
    std::string current_path = "";
    size_t pos = 0;
    while (pos < dir_path.length()) {
      size_t next_slash = dir_path.find("/", pos);
      if (next_slash == std::string::npos) {
        current_path = dir_path;
        pos = dir_path.length();
      } else {
        current_path = dir_path.substr(0, next_slash);
        pos = next_slash + 1;
      }
      
      if (!current_path.empty() && stat(current_path.c_str(), &st) != 0) {
        if (mkdir(current_path.c_str(), 0755) != 0) {
          LOG(ERROR) << "PostUploadHandler: Failed to create directory: " << current_path;
          conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
          return HR_DONE;
        }
      }
    }
  }

  // Save the POST data to file
  std::ofstream file(upload_path.c_str(), std::ios::binary);
  if (!file.is_open()) {
    LOG(ERROR) << "PostUploadHandler: Failed to open file for writing: " << upload_path;
    conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
    return HR_DONE;
  }

  const std::string& post_data = conn.request.getBody().data;
  file.write(post_data.c_str(), post_data.length());
  file.close();

  if (file.fail()) {
    LOG(ERROR) << "PostUploadHandler: Failed to write to file: " << upload_path;
    conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
    return HR_DONE;
  }

  LOG(INFO) << "PostUploadHandler: Successfully saved " << post_data.length() 
            << " bytes to " << upload_path;

  // Prepare success response
  conn.response.status_line.version = HTTP_VERSION;
  conn.response.status_line.status_code = http::S_201_CREATED;
  conn.response.status_line.reason = http::reasonPhrase(http::S_201_CREATED);

  std::ostringstream body;
  body << "POST file upload successful" << CRLF;
  body << "File saved to: " << upload_path << CRLF;
  body << "URI: " << uri << CRLF;
  body << "Content saved: " << post_data.length() << " bytes" << CRLF;
  if (!post_data.empty()) {
    body << "Data preview (first 100 chars):" << CRLF;
    body << post_data.substr(0, 100);
    if (post_data.length() > 100) {
      body << "...";
    }
  }

  conn.response.getBody().data = body.str();
  conn.response.addHeader("Content-Type", "text/plain; charset=utf-8");
  conn.response.addHeader("Location", uri);  // Standard for 201 Created

  std::ostringstream len;
  len << conn.response.getBody().size();
  conn.response.addHeader("Content-Length", len.str());

  conn.write_buffer = conn.response.serialize();

  return HR_DONE;
}

HandlerResult PostUploadHandler::resume(Connection& conn) {
  // POST upload handler doesn't need resume functionality
  (void)conn;
  return HR_ERROR;
}