#pragma once

#define DEFAULT_CONFIG_PATH "conf/default.conf"

#define CRLF "\r\n"
#define HTTP_VERSION "HTTP/1.1"

#define MAX_CONNECTIONS_PER_SERVER 10
#define MAX_EVENTS 64
#define WRITE_BUF_SIZE 4096

// Maximum bytes to scan while searching for end of headers
#define HEADERS_SEARCH_LIMIT 4096

#define EXIT_NOT_FOUND 127  // Standard shell exit code for "command not found"
#define FILE_UPLOAD_MODE \
  0600  // File permissions for uploaded files (owner read/write only)

// Connection timeout in seconds for reading client requests
// If request is not fully received within this time, respond with 408 Request
// Timeout
#define READ_TIMEOUT_SECONDS 10

// Connection timeout in seconds for writing responses to client
// If response cannot be fully sent within this time, close the connection
#define WRITE_TIMEOUT_SECONDS 10
