#pragma once

#define HTTP_VERSION "HTTP/1.1"
#define MAX_CONNECTIONS_PER_SERVER 10
#define MAX_EVENTS 64
#define WRITE_BUF_SIZE 4096
#define CRLF "\r\n"
#define DEFAULT_CONFIG_PATH "conf/default.conf"
#define EXIT_NOT_FOUND 127  // Standard shell exit code for "command not found"
#define FILE_UPLOAD_MODE 0600  // File permissions for uploaded files (owner read/write only)
