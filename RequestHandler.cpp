#include "RequestHandler.hpp"
#include "constants.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

RequestHandler::RequestHandler() {}
RequestHandler::RequestHandler(const RequestHandler &other) {
  (void)other;
}
RequestHandler &RequestHandler::operator=(const RequestHandler &other) {
  (void)other;
  return *this;
}
RequestHandler::~RequestHandler() {}

std::string RequestHandler::getMimeType(const std::string &path) {
  size_t dot_pos = path.find_last_of('.');
  if (dot_pos == std::string::npos)
    return "application/octet-stream";

  std::string ext = path.substr(dot_pos);

  if (ext == ".html" || ext == ".htm")
    return "text/html";
  if (ext == ".css")
    return "text/css";
  if (ext == ".js")
    return "application/javascript";
  if (ext == ".json")
    return "application/json";
  if (ext == ".png")
    return "image/png";
  if (ext == ".jpg" || ext == ".jpeg")
    return "image/jpeg";
  if (ext == ".gif")
    return "image/gif";
  if (ext == ".svg")
    return "image/svg+xml";
  if (ext == ".txt")
    return "text/plain";

  return "application/octet-stream";
}

bool RequestHandler::fileExists(const std::string &path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

std::string RequestHandler::readFile(const std::string &path) {
  std::ifstream file(path.c_str(), std::ios::binary);
  if (!file.is_open())
    return "";

  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

void RequestHandler::sendError(Connection &conn, int status_code,
                               const std::string &reason) {
  conn.response.status_line.version = HTTP_VERSION;
  conn.response.status_line.status_code = status_code;
  conn.response.status_line.reason = reason;

  std::ostringstream body;
  body << "<html><head><title>" << status_code << " " << reason
       << "</title></head>";
  body << "<body><h1>" << status_code << " " << reason << "</h1></body></html>";

  conn.response.setBody(Body(body.str()));

  std::ostringstream len;
  len << conn.response.getBody().size();
  conn.response.addHeader("Content-Length", len.str());
  conn.response.addHeader("Content-Type", "text/html; charset=utf-8");

  conn.write_buffer = conn.response.serialize();
}

void RequestHandler::serveStaticFile(Connection &conn, const Server &srv,
                                     const std::string &file_path) {
  // Get root from directives map
  std::string root = "./";
  std::map<std::string, std::vector<std::string> >::const_iterator root_it =
      srv.directives.find("root");
  if (root_it != srv.directives.end() && !root_it->second.empty()) {
    root = root_it->second[0];
  }

  std::string full_path = root;
  if (!full_path.empty() && full_path[full_path.size() - 1] != '/')
    full_path += "/";

  // Remove leading slash from file_path if present
  if (!file_path.empty() && file_path[0] == '/') {
    full_path += file_path.substr(1);
  } else {
    full_path += file_path;
  }

  // If path ends with /, try index file
  if (file_path.empty() || file_path[file_path.size() - 1] == '/') {
    std::map<std::string, std::vector<std::string> >::const_iterator index_it =
        srv.directives.find("index");
    if (index_it != srv.directives.end() && !index_it->second.empty()) {
      full_path += index_it->second[0];
    }
  }

  if (!fileExists(full_path)) {
    sendError(conn, 404, "Not Found");
    return;
  }

  std::string content = readFile(full_path);
  if (content.empty()) {
    sendError(conn, 500, "Internal Server Error");
    return;
  }

  conn.response.status_line.version = HTTP_VERSION;
  conn.response.status_line.status_code = 200;
  conn.response.status_line.reason = "OK";
  conn.response.setBody(Body(content));

  std::ostringstream len;
  len << conn.response.getBody().size();
  conn.response.addHeader("Content-Length", len.str());
  conn.response.addHeader("Content-Type", getMimeType(full_path));

  conn.write_buffer = conn.response.serialize();
}

void RequestHandler::handleRequest(Connection &conn, const Server &srv) {
  std::string method = conn.request.request_line.method;
  std::string uri = conn.request.request_line.uri;

  // Only handle GET for now
  if (method != "GET") {
    sendError(conn, 405, "Method Not Allowed");
    return;
  }

  // Serve the requested file
  serveStaticFile(conn, srv, uri);
}
