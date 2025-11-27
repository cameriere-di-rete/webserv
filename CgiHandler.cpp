#include "CgiHandler.hpp"

#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "Logger.hpp"
#include "constants.hpp"
#include "utils.hpp"

CgiHandler::CgiHandler(const std::string& script_path)
    : script_path_(script_path),
      script_pid_(-1),
      pipe_read_fd_(-1),
      pipe_write_fd_(-1),
      process_started_(false),
      headers_parsed_(false),
      accumulated_output_() {}

CgiHandler::~CgiHandler() {
  cleanupProcess();
}

void CgiHandler::cleanupProcess() {
  if (pipe_read_fd_ >= 0) {
    close(pipe_read_fd_);
    pipe_read_fd_ = -1;
  }
  if (pipe_write_fd_ >= 0) {
    close(pipe_write_fd_);
    pipe_write_fd_ = -1;
  }
  if (script_pid_ > 0) {
    int status;
    waitpid(script_pid_, &status, WNOHANG);
    script_pid_ = -1;
  }
}

int CgiHandler::getMonitorFd() const {
  return pipe_read_fd_;
}

HandlerResult CgiHandler::start(Connection& conn) {
  LOG(DEBUG) << "CgiHandler: starting CGI script " << script_path_;

  // Create pipes for communication
  int pipe_to_cgi[2], pipe_from_cgi[2];
  if (pipe(pipe_to_cgi) == -1 || pipe(pipe_from_cgi) == -1) {
    LOG_PERROR(ERROR, "CgiHandler: pipe failed");
    conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
    return HR_DONE;
  }

  // Fork CGI process
  script_pid_ = fork();
  if (script_pid_ == -1) {
    LOG_PERROR(ERROR, "CgiHandler: fork failed");
    close(pipe_to_cgi[0]);
    close(pipe_to_cgi[1]);
    close(pipe_from_cgi[0]);
    close(pipe_from_cgi[1]);
    conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
    return HR_DONE;
  }

  if (script_pid_ == 0) {
    // Child process - execute CGI script
    close(pipe_to_cgi[1]);    // Close write end
    close(pipe_from_cgi[0]);  // Close read end

    // Redirect stdin/stdout
    dup2(pipe_to_cgi[0], STDIN_FILENO);
    dup2(pipe_from_cgi[1], STDOUT_FILENO);
    dup2(pipe_from_cgi[1], STDERR_FILENO);

    close(pipe_to_cgi[0]);
    close(pipe_from_cgi[1]);

    // Setup environment
    setupEnvironment(conn);

    // Get absolute path before chdir
    char abs_path_buf[PATH_MAX];
    if (realpath(script_path_.c_str(), abs_path_buf) == NULL) {
      exit(1);
    }
    std::string abs_script_path = abs_path_buf;

    // Change to script directory for relative paths
    size_t last_slash = abs_script_path.find_last_of('/');
    std::string script_name;
    if (last_slash != std::string::npos) {
      std::string script_dir = abs_script_path.substr(0, last_slash);
      script_name = abs_script_path.substr(last_slash + 1);
      if (chdir(script_dir.c_str()) != 0) {
        exit(1);
      }
    } else {
      script_name = abs_script_path;
    }

    // Execute script using just the filename (we're in its directory)
    const char* script_c = script_name.c_str();
    execl(script_c, script_c, (char*)NULL);

    // If we reach here, exec failed - write error to stderr
    const char* error_msg = "CGI exec failed\n";
    write(STDERR_FILENO, error_msg, strlen(error_msg));
    exit(127);
  }

  // Parent process
  close(pipe_to_cgi[0]);    // Close read end
  close(pipe_from_cgi[1]);  // Close write end

  pipe_write_fd_ = pipe_to_cgi[1];
  pipe_read_fd_ = pipe_from_cgi[0];
  process_started_ = true;

  // Set pipe to non-blocking mode for asynchronous I/O
  if (set_nonblocking(pipe_read_fd_) < 0) {
    LOG_PERROR(ERROR, "CgiHandler: failed to set pipe non-blocking");
    conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
    cleanupProcess();
    return HR_DONE;
  }

  // Send request body to CGI if present
  const std::string& body = conn.request.getBody().data;
  if (!body.empty()) {
    ssize_t written = write(pipe_write_fd_, body.c_str(), body.length());
    if (written == -1) {
      LOG_PERROR(ERROR, "CgiHandler: write to CGI failed");
    }
  }
  close(pipe_write_fd_);
  pipe_write_fd_ = -1;

  // Start reading CGI output (non-blocking)
  return readCgiOutput(conn);
}

HandlerResult CgiHandler::resume(Connection& conn) {
  if (!process_started_) {
    return HR_ERROR;
  }
  return readCgiOutput(conn);
}

HandlerResult CgiHandler::readCgiOutput(Connection& conn) {
  char buffer[4096];
  ssize_t bytes_read;

  // Read available output from CGI (non-blocking)
  // The pipe is set to O_NONBLOCK, so read() will return -1 with EAGAIN
  // when no data is available, preventing blocking
  while ((bytes_read = read(pipe_read_fd_, buffer, sizeof(buffer))) > 0) {
    accumulated_output_.append(buffer, bytes_read);
  }

  if (bytes_read == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // No more data available right now, need to wait for more
      LOG(DEBUG) << "CgiHandler: would block, accumulated "
                 << accumulated_output_.size() << " bytes so far";
      return HR_WOULD_BLOCK;
    }
    // Real error
    LOG_PERROR(ERROR, "CgiHandler: read from CGI failed");
    cleanupProcess();
    conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
    return HR_DONE;
  }

  // bytes_read == 0 means EOF - CGI finished writing
  LOG(DEBUG) << "CgiHandler: CGI finished, total output: "
             << accumulated_output_.size() << " bytes";

  // Close the pipe read fd since we're done reading
  close(pipe_read_fd_);
  pipe_read_fd_ = -1;

  // Wait for process to finish
  int status;
  waitpid(script_pid_, &status, 0);
  script_pid_ = -1;

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    LOG(ERROR) << "CGI script exited with error status: "
               << WEXITSTATUS(status);
    conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);
    return HR_DONE;
  }

  // Process all output data
  if (!accumulated_output_.empty()) {
    parseOutput(conn, accumulated_output_);
  }

  // Ensure we have response headers if none were parsed
  if (!headers_parsed_) {
    conn.response.status_line.version = HTTP_VERSION;
    conn.response.status_line.status_code = http::S_200_OK;
    conn.response.status_line.reason = "OK";
    conn.response.addHeader("Content-Type", "text/plain");

    std::ostringstream response_stream;
    response_stream << conn.response.startLine() << CRLF;
    response_stream << conn.response.serializeHeaders();
    response_stream << CRLF;
    response_stream << accumulated_output_;

    conn.write_buffer = response_stream.str();
  }

  LOG(DEBUG) << "CGI finished, response size: " << conn.write_buffer.size();
  return HR_DONE;
}

HandlerResult CgiHandler::parseOutput(Connection& conn,
                                      const std::string& data) {
  if (!headers_parsed_) {
    remaining_data_ += data;

    // Look for headers end
    size_t headers_end = remaining_data_.find(CRLF CRLF);
    if (headers_end == std::string::npos) {
      return HR_WOULD_BLOCK;  // Need more data
    }

    // Parse headers
    std::string headers_part = remaining_data_.substr(0, headers_end);
    std::string body_part = remaining_data_.substr(headers_end + 4);

    // Set default status
    conn.response.status_line.version = HTTP_VERSION;
    conn.response.status_line.status_code = http::S_200_OK;
    conn.response.status_line.reason = "OK";

    // Parse header lines
    std::istringstream iss(headers_part);
    std::string line;
    while (std::getline(iss, line) && !line.empty()) {
      if (!line.empty() && line[line.length() - 1] == '\r') {
        line.erase(line.length() - 1);
      }

      size_t colon = line.find(':');
      if (colon != std::string::npos) {
        std::string name = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        // Trim whitespace
        while (!value.empty() && value[0] == ' ') {
          value.erase(0, 1);
        }

        if (name == "Status") {
          // Parse status line
          size_t space = value.find(' ');
          if (space != std::string::npos) {
            int status_code = atoi(value.substr(0, space).c_str());
            conn.response.status_line.status_code =
                static_cast<http::Status>(status_code);
            conn.response.status_line.reason = value.substr(space + 1);
          }
        } else {
          conn.response.addHeader(name, value);
        }
      }
    }

    headers_parsed_ = true;
    remaining_data_ = body_part;

    // Build response headers
    std::ostringstream response_stream;
    response_stream << conn.response.startLine() << CRLF;
    response_stream << conn.response.serializeHeaders();
    response_stream << CRLF;

    conn.write_buffer = response_stream.str();

    if (!body_part.empty()) {
      conn.write_buffer += body_part;
    }

    remaining_data_.clear();
  } else {
    // Headers already parsed, just add body data
    conn.write_buffer += data;
  }

  return HR_WOULD_BLOCK;
}

void CgiHandler::setupEnvironment(Connection& conn) {
  // Set PATH for script execution
  setenv("PATH", "/usr/local/bin:/usr/bin:/bin", 1);

  // Standard CGI environment variables
  setenv("REQUEST_METHOD", conn.request.request_line.method.c_str(), 1);
  setenv("REQUEST_URI", conn.request.request_line.uri.c_str(), 1);
  setenv("SERVER_PROTOCOL", conn.request.request_line.version.c_str(), 1);
  setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
  setenv("SERVER_NAME", "webserv", 1);
  setenv("SERVER_PORT", "8080", 1);
  setenv("SCRIPT_NAME", script_path_.c_str(), 1);

  // Query string
  std::string uri = conn.request.request_line.uri;
  size_t query_pos = uri.find('?');
  if (query_pos != std::string::npos) {
    setenv("QUERY_STRING", uri.substr(query_pos + 1).c_str(), 1);
    setenv("PATH_INFO", uri.substr(0, query_pos).c_str(), 1);
  } else {
    setenv("QUERY_STRING", "", 1);
    setenv("PATH_INFO", uri.c_str(), 1);
  }

  // Content headers
  std::string content_type, content_length;
  if (conn.request.getHeader("Content-Type", content_type)) {
    setenv("CONTENT_TYPE", content_type.c_str(), 1);
  }
  if (conn.request.getHeader("Content-Length", content_length)) {
    setenv("CONTENT_LENGTH", content_length.c_str(), 1);
  } else {
    std::ostringstream len_ss;
    len_ss << conn.request.getBody().data.length();
    setenv("CONTENT_LENGTH", len_ss.str().c_str(), 1);
  }

  // HTTP headers as environment variables
  // Note: Request class needs to provide header iteration interface
}