/**
 * @file Connection_test.cpp
 * @brief Unit tests for Connection class: HTTP validation and request body
 * limits
 *
 * These tests verify that the server correctly:
 * - Validates HTTP versions (1.0, 1.1 accepted; others rejected with 505)
 * - Rejects requests with bodies exceeding max_request_body limit (413)
 * - Prepares error responses with correct status codes and versions
 */

#include "Connection.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
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

// =============================================================================
// Helper: Create a Location with specific max_request_body
// =============================================================================

// Helper: Create a Location with specific max_request_body
static Location createLocationWithMaxBody(std::size_t max_body) {
  Location loc("/");
  loc.root = "/tmp";
  loc.allow_methods.insert(http::GET);
  loc.allow_methods.insert(http::POST);
  loc.max_request_body = max_body;
  return loc;
}

// =============================================================================
// HTTP Version Validation Tests
// =============================================================================

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

TEST(ConnectionTests, ErrorResponseUsesRequestVersion) {
  Connection conn;
  conn.request.request_line.version = "HTTP/1.0";
  conn.request.request_line.method = "GET";

  conn.prepareErrorResponse(http::S_404_NOT_FOUND);

  EXPECT_EQ(conn.response.status_line.version, "HTTP/1.0");
  EXPECT_EQ(conn.response.status_line.status_code, http::S_404_NOT_FOUND);
}

TEST(ConnectionTests, ErrorResponseDefaultsToHttp11) {
  Connection conn;
  conn.request.request_line.version = "";

  conn.prepareErrorResponse(http::S_500_INTERNAL_SERVER_ERROR);

  EXPECT_EQ(conn.response.status_line.version, "HTTP/1.1");
  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_500_INTERNAL_SERVER_ERROR);
}

TEST(ConnectionTests, ErrorResponseForUnsupportedVersionUsesHttp11) {
  Connection conn;
  conn.request.request_line.version = "HTTP/2.0";

  conn.prepareErrorResponse(http::S_505_HTTP_VERSION_NOT_SUPPORTED);

  EXPECT_EQ(conn.response.status_line.version, "HTTP/1.1");
  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_505_HTTP_VERSION_NOT_SUPPORTED);
}

// =============================================================================
// Custom Error Page Tests (recovered from origin/main)
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

  conn.prepareErrorResponse(http::S_404_NOT_FOUND);

  // Status code should still be 404 (overridden from FileHandler's 200)
  EXPECT_EQ(conn.response.status_line.status_code, http::S_404_NOT_FOUND);
  EXPECT_EQ(conn.response.status_line.reason, "Not Found");

  // error_pages should be restored after serving
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

// =============================================================================
// Max Request Body Validation Tests
// =============================================================================

TEST(MaxRequestBodyValidation, BodyExceedsLimitReturns413) {
  Connection conn;
  conn.request.request_line.method = "POST";
  conn.request.request_line.uri = "/upload";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(1000, 'X');  // 1000 bytes body

  Location loc = createLocationWithMaxBody(500);  // Limit is 500 bytes

  conn.processResponse(loc);

  // Should have prepared a 413 response
  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
  EXPECT_FALSE(conn.write_buffer.empty());
  EXPECT_NE(conn.write_buffer.find("413"), std::string::npos);
}

TEST(MaxRequestBodyValidation, BodyExceedsLimitByOne) {
  Connection conn;
  conn.request.request_line.method = "POST";
  conn.request.request_line.uri = "/upload";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(101, 'X');  // 101 bytes

  Location loc = createLocationWithMaxBody(100);  // Limit is 100 bytes

  conn.processResponse(loc);

  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

// =============================================================================
// Test: Request body exactly at limit should be allowed
// =============================================================================

TEST(MaxRequestBodyValidation, BodyExactlyAtLimitIsAllowed) {
  Connection conn;
  conn.request.request_line.method = "GET";
  conn.request.request_line.uri = "/";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(100, 'X');  // Exactly 100 bytes

  Location loc = createLocationWithMaxBody(100);  // Limit is 100 bytes

  conn.processResponse(loc);

  // Should NOT be 413 (may be 404 since file doesn't exist, but not 413)
  EXPECT_NE(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

TEST(MaxRequestBodyValidation, BodyBelowLimitIsAllowed) {
  Connection conn;
  conn.request.request_line.method = "GET";
  conn.request.request_line.uri = "/";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(50, 'X');  // 50 bytes

  Location loc = createLocationWithMaxBody(100);  // Limit is 100 bytes

  conn.processResponse(loc);

  // Should NOT be 413
  EXPECT_NE(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

// =============================================================================
// Test: Empty body should always be allowed
// =============================================================================

TEST(MaxRequestBodyValidation, EmptyBodyIsAllowed) {
  Connection conn;
  conn.request.request_line.method = "GET";
  conn.request.request_line.uri = "/";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = "";  // Empty body

  Location loc = createLocationWithMaxBody(100);

  conn.processResponse(loc);

  EXPECT_NE(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

TEST(MaxRequestBodyValidation, EmptyBodyWithZeroLimitIsAllowed) {
  Connection conn;
  conn.request.request_line.method = "GET";
  conn.request.request_line.uri = "/";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = "";  // Empty body

  Location loc = createLocationWithMaxBody(0);  // Zero limit

  conn.processResponse(loc);

  // Empty body (0 bytes) should pass even with limit of 0
  // because 0 > 0 is false
  EXPECT_NE(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

// =============================================================================
// Test: Zero limit should reject any non-empty body
// =============================================================================

TEST(MaxRequestBodyValidation, ZeroLimitRejectsNonEmptyBody) {
  Connection conn;
  conn.request.request_line.method = "POST";
  conn.request.request_line.uri = "/";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = "x";  // 1 byte body

  Location loc = createLocationWithMaxBody(0);  // Zero limit

  conn.processResponse(loc);

  // 1 > 0 is true, so should be rejected
  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

// =============================================================================
// Test: Unset max_request_body (sentinel value) allows any body size
// =============================================================================

TEST(MaxRequestBodyValidation, UnsetLimitAllowsAnyBodySize) {
  Connection conn;
  conn.request.request_line.method = "POST";
  conn.request.request_line.uri = "/";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(1000000, 'X');  // 1MB body

  Location loc = createLocationWithMaxBody(kMaxRequestBodyUnset);

  conn.processResponse(loc);

  // With unset limit, body size check should be skipped
  EXPECT_NE(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

// =============================================================================
// Test: Large body with large limit
// =============================================================================

TEST(MaxRequestBodyValidation, LargeBodyWithLargeLimitIsAllowed) {
  Connection conn;
  conn.request.request_line.method = "POST";
  conn.request.request_line.uri = "/";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(10000, 'X');  // 10KB body

  Location loc = createLocationWithMaxBody(1048576);  // 1MB limit

  conn.processResponse(loc);

  EXPECT_NE(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

TEST(MaxRequestBodyValidation, LargeBodyExceedsLargeLimit) {
  Connection conn;
  conn.request.request_line.method = "POST";
  conn.request.request_line.uri = "/upload";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(2000000, 'X');  // 2MB body

  Location loc = createLocationWithMaxBody(1048576);  // 1MB limit

  conn.processResponse(loc);

  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

// =============================================================================
// Test: Different HTTP methods with body limits
// =============================================================================

TEST(MaxRequestBodyValidation, PostMethodWithExcessiveBody) {
  Connection conn;
  conn.request.request_line.method = "POST";
  conn.request.request_line.uri = "/api/data";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(200, 'X');

  Location loc = createLocationWithMaxBody(100);

  conn.processResponse(loc);

  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

TEST(MaxRequestBodyValidation, PutMethodWithExcessiveBody) {
  Connection conn;
  conn.request.request_line.method = "PUT";
  conn.request.request_line.uri = "/files/test.txt";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(500, 'X');

  Location loc = createLocationWithMaxBody(100);
  loc.allow_methods.insert(http::PUT);

  conn.processResponse(loc);

  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

TEST(MaxRequestBodyValidation, GetMethodWithExcessiveBody) {
  // GET requests can technically have a body (though uncommon)
  Connection conn;
  conn.request.request_line.method = "GET";
  conn.request.request_line.uri = "/search";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(200, 'X');

  Location loc = createLocationWithMaxBody(100);

  conn.processResponse(loc);

  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}

// =============================================================================
// Test: 413 response contains correct status text
// =============================================================================

TEST(MaxRequestBodyValidation, Response413HasCorrectReasonPhrase) {
  Connection conn;
  conn.request.request_line.method = "POST";
  conn.request.request_line.uri = "/upload";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(1000, 'X');

  Location loc = createLocationWithMaxBody(100);

  conn.processResponse(loc);

  EXPECT_EQ(conn.response.status_line.reason, "Payload Too Large");
}

TEST(MaxRequestBodyValidation, Response413HasHtmlBody) {
  Connection conn;
  conn.request.request_line.method = "POST";
  conn.request.request_line.uri = "/upload";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(1000, 'X');

  Location loc = createLocationWithMaxBody(100);

  conn.processResponse(loc);

  // write_buffer should contain HTML error page
  EXPECT_NE(conn.write_buffer.find("<html>"), std::string::npos);
  EXPECT_NE(conn.write_buffer.find("413"), std::string::npos);
  EXPECT_NE(conn.write_buffer.find("Payload Too Large"), std::string::npos);
}

// =============================================================================
// Test: Body check happens before other processing
// =============================================================================

TEST(MaxRequestBodyValidation, BodyCheckHappensEarlyInProcessing) {
  // Even if the path doesn't exist, we should get 413 first
  Connection conn;
  conn.request.request_line.method = "POST";
  conn.request.request_line.uri = "/nonexistent/path/that/does/not/exist";
  conn.request.request_line.version = "HTTP/1.1";
  conn.request.getBody().data = std::string(1000, 'X');

  Location loc = createLocationWithMaxBody(100);

  conn.processResponse(loc);

  // Should be 413, not 404
  EXPECT_EQ(conn.response.status_line.status_code,
            http::S_413_PAYLOAD_TOO_LARGE);
}
