#include "Connection.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

#include "HttpStatus.hpp"
#include "Location.hpp"
#include "utils.hpp"

// Helper to create a temporary file with given content
static std::string createTempFile(const std::string& content) {
  char tmpl[] = "/tmp/webserv_error_page_XXXXXX";
  int fd = mkstemp(tmpl);
  if (fd < 0) {
    return "";
  }
  ssize_t w = write(fd, content.c_str(), content.size());
  (void)w;
  fsync(fd);
  close(fd);
  return std::string(tmpl);
}

TEST(ConnectionTests, DefaultConstructorInitializesFields) {
  Connection c;
  EXPECT_EQ(c.fd, -1);
  EXPECT_EQ(c.server_fd, -1);
  EXPECT_TRUE(c.read_buffer.empty());
  EXPECT_TRUE(c.write_buffer.empty());
  EXPECT_EQ(c.write_offset, 0u);
  EXPECT_EQ(c.headers_end_pos, std::string::npos);  // Initialized to npos
  EXPECT_FALSE(c.write_ready);
  EXPECT_EQ(c.active_handler, static_cast<IHandler*>(NULL));
}

TEST(ConnectionTests, ParameterizedConstructorSetsFd) {
  Connection c(42);
  EXPECT_EQ(c.fd, 42);
  EXPECT_EQ(c.server_fd, -1);
}

TEST(ConnectionTests, CopyConstructorCopiesFields) {
  Connection c1(10);
  c1.server_fd = 5;
  c1.read_buffer = "test data";
  c1.write_ready = true;

  Connection c2(c1);
  EXPECT_EQ(c2.fd, 10);
  EXPECT_EQ(c2.server_fd, 5);
  EXPECT_EQ(c2.read_buffer, "test data");
  EXPECT_TRUE(c2.write_ready);
}

TEST(ConnectionTests, AssignmentOperatorCopiesFields) {
  Connection c1(20);
  c1.server_fd = 15;
  c1.write_buffer = "response";
  c1.write_offset = 5;

  Connection c2;
  c2 = c1;
  EXPECT_EQ(c2.fd, 20);
  EXPECT_EQ(c2.server_fd, 15);
  EXPECT_EQ(c2.write_buffer, "response");
  EXPECT_EQ(c2.write_offset, 5u);
}

TEST(ConnectionTests, ActiveHandlerIsNull) {
  Connection c;
  EXPECT_EQ(c.active_handler, static_cast<IHandler*>(NULL));
}

// Test that HTTP/1.1 requests are accepted
TEST(ConnectionTests, AcceptsHttp11Requests) {
  Connection conn;
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.request_line.method = "GET";

  Location location;
  location.path = "/";
  initDefaultHttpMethods(location.allow_methods);

  http::Status status = conn.validateRequestForLocation(location);

  // S_0_UNKNOWN (0) indicates validation success
  EXPECT_EQ(status, http::S_0_UNKNOWN);
}

// Test that HTTP/1.0 requests are accepted
TEST(ConnectionTests, AcceptsHttp10Requests) {
  Connection conn;
  conn.request.request_line.version = "HTTP/1.0";
  conn.request.request_line.method = "GET";

  Location location;
  location.path = "/";
  initDefaultHttpMethods(location.allow_methods);

  http::Status status = conn.validateRequestForLocation(location);

  // S_0_UNKNOWN (0) indicates validation success
  EXPECT_EQ(status, http::S_0_UNKNOWN);
}

// Test that other HTTP versions are rejected
TEST(ConnectionTests, RejectsOtherHttpVersions) {
  Connection conn;
  conn.request.request_line.version = "HTTP/2.0";
  conn.request.request_line.method = "GET";

  Location location;
  location.path = "/";
  initDefaultHttpMethods(location.allow_methods);

  http::Status status = conn.validateRequestForLocation(location);

  EXPECT_EQ(status, http::S_505_HTTP_VERSION_NOT_SUPPORTED);
}

// Test that invalid HTTP versions are rejected
TEST(ConnectionTests, RejectsInvalidHttpVersions) {
  Connection conn;
  conn.request.request_line.version = "HTTP/1.2";
  conn.request.request_line.method = "GET";

  Location location;
  location.path = "/";
  initDefaultHttpMethods(location.allow_methods);

  http::Status status = conn.validateRequestForLocation(location);

  EXPECT_EQ(status, http::S_505_HTTP_VERSION_NOT_SUPPORTED);
}

// Test that error responses use the same HTTP version as the request
TEST(ConnectionTests, ErrorResponseUsesRequestVersion) {
  Connection conn;
  conn.request.request_line.version = "HTTP/1.0";
  conn.request.request_line.method = "GET";

  conn.prepareErrorResponse(http::S_404_NOT_FOUND);

  EXPECT_EQ(conn.response.status_line.version, "HTTP/1.0");
  EXPECT_EQ(conn.response.status_line.status_code, http::S_404_NOT_FOUND);
}

// Test that error responses default to HTTP/1.1 when request version is empty
TEST(ConnectionTests, ErrorResponseDefaultsToHttp11) {
  Connection conn;
  conn.request.request_line.version = "";

  conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);

  EXPECT_EQ(conn.response.status_line.version, "HTTP/1.1");
  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_500_INTERNAL_SERVER_ERROR);
}

// Test that error responses for unsupported HTTP versions use HTTP/1.1
TEST(ConnectionTests, ErrorResponseForUnsupportedVersionUsesHttp11) {
  Connection conn;
  conn.request.request_line.version = "HTTP/2.0";

  conn.prepareErrorResponse(http::S_505_HTTP_VERSION_NOT_SUPPORTED);

  EXPECT_EQ(conn.response.status_line.version, "HTTP/1.1");
  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_505_HTTP_VERSION_NOT_SUPPORTED);
}

// =============================================================================
// Custom Error Page Tests
// =============================================================================

// Test: Default error response (no custom error page configured)
TEST(ConnectionErrorPageTests, DefaultErrorResponse) {
  Connection conn;
  conn.prepareErrorResponse(http::S_404_NOT_FOUND);

  // Should have correct status code
  EXPECT_EQ(conn.response.status_line.status_code, http::S_404_NOT_FOUND);
  EXPECT_EQ(conn.response.status_line.reason, "Not Found");

  // Should have default HTML body with title
  EXPECT_FALSE(conn.response.getBody().data.empty());
  EXPECT_NE(conn.response.getBody().data.find("404 Not Found"),
            std::string::npos);

  // Should have Content-Type header
  std::string content_type;
  EXPECT_TRUE(conn.response.getHeader("Content-Type", content_type));
  EXPECT_EQ(content_type, "text/html; charset=utf-8");

  // write_buffer should be set
  EXPECT_FALSE(conn.write_buffer.empty());
}

// Test: Custom error page successfully served
TEST(ConnectionErrorPageTests, CustomErrorPageSuccess) {
  // Create a custom error page file
  std::string custom_content = "<html><body>Custom 404 Page</body></html>";
  std::string custom_path = createTempFile(custom_content);
  ASSERT_FALSE(custom_path.empty());
  Connection conn;
  // Set up a GET request (FileHandler needs this)
  conn.request.request_line.method = "GET";
  conn.error_pages[http::S_404_NOT_FOUND] = custom_path;

  // Use a socketpair so the handler can send headers/body to a real fd
  int sv[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
  // Use sv[1] as the connection fd
  conn.fd = sv[1];

  conn.prepareErrorResponse(http::S_404_NOT_FOUND);

  // If a handler was installed for streaming, drive it to completion.
  if (conn.active_handler) {
    // First flush any headers prepared by start()
    while (conn.write_offset < conn.write_buffer.size()) {
      int hr = conn.handleWrite();
      if (hr < 0) {
        break;
      }
    }

    // Resume the handler until done
    while (conn.active_handler) {
      HandlerResult r = conn.active_handler->resume(conn);
      // After resume, flush any headers (if present)
      while (conn.write_offset < conn.write_buffer.size()) {
        int hr = conn.handleWrite();
        if (hr < 0) {
          break;
        }
      }
      if (r == HR_DONE) {
        break;
      }
      if (r == HR_ERROR) {
        break;
      }
    }
  } else {
    // No active handler: headers/body should already be prepared
  }

  // Read the full response from the peer socket
  std::string received;
  char buf[1024];
  ssize_t n = 0;
  // Close our write end to ensure EOF for the reader
  shutdown(sv[1], SHUT_WR);
  while ((n = read(sv[0], buf, sizeof(buf))) > 0) {
    received.append(buf, n);
  }
  close(sv[0]);
  close(sv[1]);

  // Check status line and headers present
  EXPECT_NE(received.find("HTTP/1.1 404 Not Found"), std::string::npos);
  EXPECT_NE(received.find("Content-Length"), std::string::npos);
  // Body should contain our custom content
  EXPECT_NE(received.find(custom_content), std::string::npos);

  // error_pages should still reference the custom path
  EXPECT_FALSE(conn.error_pages.empty());
  EXPECT_EQ(conn.error_pages[http::S_404_NOT_FOUND], custom_path);

  // Cleanup
  unlink(custom_path.c_str());
}

// Test: Fallback to default error page when custom file is missing
TEST(ConnectionErrorPageTests, FallbackWhenCustomFileMissing) {
  Connection conn;
  // Set up a GET request
  conn.request.request_line.method = "GET";
  // Point to a non-existent file
  conn.error_pages[http::S_404_NOT_FOUND] = "/nonexistent/path/404.html";

  conn.prepareErrorResponse(http::S_404_NOT_FOUND);

  // Should fall back to default error page
  EXPECT_EQ(conn.response.status_line.status_code, http::S_404_NOT_FOUND);
  EXPECT_EQ(conn.response.status_line.reason, "Not Found");

  // Should have default HTML body (not empty, contains status)
  EXPECT_FALSE(conn.response.getBody().data.empty());
  EXPECT_NE(conn.response.getBody().data.find("404 Not Found"),
            std::string::npos);

  // error_pages should be restored even after failure
  EXPECT_FALSE(conn.error_pages.empty());
  EXPECT_EQ(conn.error_pages[http::S_404_NOT_FOUND],
            "/nonexistent/path/404.html");
}

// Test: No infinite recursion when custom error page for 404 is missing
// (The 404 handler would normally trigger another 404 for the missing file)
TEST(ConnectionErrorPageTests, NoInfiniteRecursionOnMissingErrorPage) {
  Connection conn;
  conn.request.request_line.method = "GET";
  // Set a 404 error page that doesn't exist - this should NOT cause recursion
  conn.error_pages[http::S_404_NOT_FOUND] = "/missing/404.html";

  // This should complete without stack overflow
  conn.prepareErrorResponse(http::S_404_NOT_FOUND);

  // Should fall back to default and complete
  EXPECT_EQ(conn.response.status_line.status_code, http::S_404_NOT_FOUND);
  EXPECT_FALSE(conn.write_buffer.empty());
}

// Test: error_pages is restored after successful custom error page
TEST(ConnectionErrorPageTests, ErrorPagesRestoredAfterSuccess) {
  std::string custom_content = "<html><body>Error</body></html>";
  std::string custom_path = createTempFile(custom_content);
  ASSERT_FALSE(custom_path.empty());

  Connection conn;
  conn.request.request_line.method = "GET";
  conn.error_pages[http::S_404_NOT_FOUND] = custom_path;
  conn.error_pages[http::S_500_INTERNAL_SERVER_ERROR] = "/other/500.html";

  conn.prepareErrorResponse(http::S_404_NOT_FOUND);

  // Both error pages should still be in the map
  EXPECT_EQ(conn.error_pages.size(), 2u);
  EXPECT_EQ(conn.error_pages[http::S_404_NOT_FOUND], custom_path);
  EXPECT_EQ(conn.error_pages[http::S_500_INTERNAL_SERVER_ERROR],
            "/other/500.html");

  unlink(custom_path.c_str());
}

// Test: error_pages is restored after failed custom error page
TEST(ConnectionErrorPageTests, ErrorPagesRestoredAfterFailure) {
  Connection conn;
  conn.request.request_line.method = "GET";
  conn.error_pages[http::S_404_NOT_FOUND] = "/missing/404.html";
  conn.error_pages[http::S_500_INTERNAL_SERVER_ERROR] = "/other/500.html";

  conn.prepareErrorResponse(http::S_404_NOT_FOUND);

  // Both error pages should still be in the map after fallback
  EXPECT_EQ(conn.error_pages.size(), 2u);
  EXPECT_EQ(conn.error_pages[http::S_404_NOT_FOUND], "/missing/404.html");
  EXPECT_EQ(conn.error_pages[http::S_500_INTERNAL_SERVER_ERROR],
            "/other/500.html");
}

// Test: Different error status uses different error page
TEST(ConnectionErrorPageTests, DifferentStatusUsesDifferentPage) {
  std::string content_404 = "<html><body>404 Custom</body></html>";
  std::string content_500 = "<html><body>500 Custom</body></html>";
  std::string path_404 = createTempFile(content_404);
  std::string path_500 = createTempFile(content_500);
  ASSERT_FALSE(path_404.empty());
  ASSERT_FALSE(path_500.empty());

  Connection conn;
  conn.request.request_line.method = "GET";
  conn.error_pages[http::S_404_NOT_FOUND] = path_404;
  conn.error_pages[http::S_500_INTERNAL_SERVER_ERROR] = path_500;

  // Request 500 error
  conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);

  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_500_INTERNAL_SERVER_ERROR);
  EXPECT_EQ(conn.response.status_line.reason, "Internal Server Error");

  unlink(path_404.c_str());
  unlink(path_500.c_str());
}

// Test: Status without configured error page uses default
TEST(ConnectionErrorPageTests, UnconfiguredStatusUsesDefault) {
  std::string content_404 = "<html><body>404 Custom</body></html>";
  std::string path_404 = createTempFile(content_404);
  ASSERT_FALSE(path_404.empty());

  Connection conn;
  conn.request.request_line.method = "GET";
  conn.error_pages[http::S_404_NOT_FOUND] = path_404;

  // Request 403 error (not configured)
  conn.prepareErrorResponse(http::S_403_FORBIDDEN);

  EXPECT_EQ(conn.response.status_line.status_code, http::S_403_FORBIDDEN);
  EXPECT_EQ(conn.response.status_line.reason, "Forbidden");

  // Should have default HTML body
  EXPECT_NE(conn.response.getBody().data.find("403 Forbidden"),
            std::string::npos);

  unlink(path_404.c_str());
}
