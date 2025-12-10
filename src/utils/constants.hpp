#pragma once

#define HTTP_VERSION "HTTP/1.1"
#define MAX_CONNECTIONS_PER_SERVER 10
#define MAX_EVENTS 64
#define WRITE_BUF_SIZE 4096
#define CRLF "\r\n"
#define DEFAULT_CONFIG_PATH "conf/default.conf"
#define EXIT_NOT_FOUND 127  // Standard shell exit code for "command not found"

// Connection timeout in seconds for reading client requests
// If request is not fully received within this time, respond with 408 Request Timeout
#define READ_TIMEOUT_SECONDS 30

// Connection timeout in seconds for writing responses to client
// If response cannot be fully sent within this time, close the connection
#define WRITE_TIMEOUT_SECONDS 30
