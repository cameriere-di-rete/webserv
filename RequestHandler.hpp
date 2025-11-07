#pragma once

#include "Connection.hpp"
#include "Server.hpp"
#include <string>

class RequestHandler {
public:
  // Handles HTTP request using server configuration
  // Routes to appropriate handler based on method and URI
  static void handleRequest(Connection &conn, const Server &srv);

private:
  RequestHandler();
  RequestHandler(const RequestHandler &other);
  RequestHandler &operator=(const RequestHandler &other);
  ~RequestHandler();

  // Serves a static file from filesystem using server's root directive
  // Handles index files and MIME type detection
  static void serveStaticFile(Connection &conn, const Server &srv,
                               const std::string &file_path);

  // Sends HTTP error response with given status code and reason
  static void sendError(Connection &conn, int status_code,
                        const std::string &reason);

  // Determines MIME type from file extension
  static std::string getMimeType(const std::string &path);

  // Reads entire file content into string
  static std::string readFile(const std::string &path);

  // Checks if path exists and is a regular file
  static bool fileExists(const std::string &path);
};
