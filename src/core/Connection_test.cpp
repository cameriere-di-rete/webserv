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
#include "constants.hpp"
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

// Helper: Create a Location with specific max_request_body
static Location createLocationWithMaxBody(std::size_t max_body) {
  Location loc("/");
  loc.root = "/tmp";
  loc.allow_methods.insert(http::GET);
  loc.allow_methods.insert(http::POST);
  loc.max_request_body = max_body;
  return loc;
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
// HTTP Version Validation Tests
// =============================================================================

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
// Test: Connection should have read_start timestamp
// =============================================================================

TEST(ConnectionTimeout, ConnectionShouldHaveReadStartField) {
  Connection conn;
  // Connection should have a read_start field of type time_t
  EXPECT_TRUE((std::is_same<decltype(conn.read_start), time_t>::value));
}

TEST(ConnectionTimeout, ConnectionShouldHaveWriteStartField) {
  Connection conn;
  // Connection should have a write_start field of type time_t
  EXPECT_TRUE((std::is_same<decltype(conn.write_start), time_t>::value));
}

TEST(ConnectionTimeout, ConnectionShouldInitializeReadStartOnConstruction) {
  time_t before = time(NULL);
  Connection conn;
  time_t after = time(NULL);

  // read_start should be set to current time on construction
  EXPECT_GE(conn.read_start, before);
  EXPECT_LE(conn.read_start, after);
}

TEST(ConnectionTimeout, ConnectionShouldInitializeWriteStartToZero) {
  Connection conn;
  // write_start should be 0 (not started) on construction
  EXPECT_EQ(conn.write_start, 0);
}

TEST(ConnectionTimeout, ConnectionFdConstructorShouldInitializeReadStart) {
  time_t before = time(NULL);
  Connection conn(42);  // fd constructor
  time_t after = time(NULL);

  EXPECT_GE(conn.read_start, before);
  EXPECT_LE(conn.read_start, after);
  EXPECT_EQ(conn.write_start, 0);
}

TEST(ConnectionTimeout, CopyConstructorShouldCopyReadStart) {
  Connection conn1;
  conn1.read_start = 12345;

  Connection conn2(conn1);

  EXPECT_EQ(conn2.read_start, 12345);
}

TEST(ConnectionTimeout, CopyConstructorShouldCopyWriteStart) {
  Connection conn1;
  conn1.write_start = 67890;

  Connection conn2(conn1);

  EXPECT_EQ(conn2.write_start, 67890);
}

TEST(ConnectionTimeout, AssignmentOperatorShouldCopyReadStart) {
  Connection conn1;
  conn1.read_start = 12345;

  Connection conn2;
  conn2 = conn1;

  EXPECT_EQ(conn2.read_start, 12345);
}

TEST(ConnectionTimeout, AssignmentOperatorShouldCopyWriteStart) {
  Connection conn1;
  conn1.write_start = 67890;

  Connection conn2;
  conn2 = conn1;

  EXPECT_EQ(conn2.write_start, 67890);
}

// =============================================================================
// Test: startWritePhase should set write_start timestamp
// =============================================================================

TEST(ConnectionTimeout, StartWritePhaseShouldExist) {
  Connection conn;
  EXPECT_EQ(conn.write_start, 0);

  conn.startWritePhase();

  EXPECT_GT(conn.write_start, 0);
}

TEST(ConnectionTimeout, StartWritePhaseShouldSetCurrentTime) {
  Connection conn;

  time_t before = time(NULL);
  conn.startWritePhase();
  time_t after = time(NULL);

  EXPECT_GE(conn.write_start, before);
  EXPECT_LE(conn.write_start, after);
}

// =============================================================================
// Test: Read timeout detection
// =============================================================================

TEST(ConnectionTimeout, IsReadTimedOutShouldExist) {
  Connection conn;
  // Connection should have isReadTimedOut(timeout_seconds) method
  bool result = conn.isReadTimedOut(30);
  EXPECT_FALSE(result);  // Just created, should not be timed out
}

TEST(ConnectionTimeout, IsReadTimedOutReturnsFalseWhenWithinTimeout) {
  Connection conn;
  // read_start is set to now on construction
  EXPECT_FALSE(conn.isReadTimedOut(30));
  EXPECT_FALSE(conn.isReadTimedOut(60));
  EXPECT_FALSE(conn.isReadTimedOut(1));
}

TEST(ConnectionTimeout, IsReadTimedOutReturnsTrueWhenExpired) {
  Connection conn;
  conn.read_start = time(NULL) - 100;  // 100 seconds ago

  EXPECT_TRUE(conn.isReadTimedOut(30));    // 30 sec timeout -> expired
  EXPECT_TRUE(conn.isReadTimedOut(60));    // 60 sec timeout -> expired
  EXPECT_TRUE(conn.isReadTimedOut(99));    // 99 sec timeout -> expired
  EXPECT_FALSE(conn.isReadTimedOut(101));  // 101 sec timeout -> not expired
}

TEST(ConnectionTimeout, IsReadTimedOutEdgeCaseExactlyAtTimeout) {
  Connection conn;
  conn.read_start = time(NULL) - 30;  // Exactly 30 seconds ago

  // At exactly timeout boundary, should be considered timed out
  EXPECT_TRUE(conn.isReadTimedOut(30));
}

TEST(ConnectionTimeout, IsReadTimedOutWithZeroTimeoutAlwaysTimesOut) {
  Connection conn;
  // Zero timeout means immediate timeout
  EXPECT_TRUE(conn.isReadTimedOut(0));
}

TEST(ConnectionTimeout, IsReadTimedOutWithVeryOldTimestamp) {
  Connection conn;
  conn.read_start = 0;  // Unix epoch - very old

  EXPECT_TRUE(conn.isReadTimedOut(1));
  EXPECT_TRUE(conn.isReadTimedOut(86400));  // 24 hours
}

// =============================================================================
// Test: Write timeout detection
// =============================================================================

TEST(ConnectionTimeout, IsWriteTimedOutShouldExist) {
  Connection conn;
  // Connection should have isWriteTimedOut(timeout_seconds) method
  bool result = conn.isWriteTimedOut(30);
  // write_start is 0, so write phase hasn't started -> false
  EXPECT_FALSE(result);
}

TEST(ConnectionTimeout, IsWriteTimedOutReturnsFalseWhenWriteNotStarted) {
  Connection conn;
  // write_start is 0 on construction
  EXPECT_EQ(conn.write_start, 0);
  EXPECT_FALSE(conn.isWriteTimedOut(30));
  EXPECT_FALSE(conn.isWriteTimedOut(1));
  EXPECT_FALSE(conn.isWriteTimedOut(0));
}

TEST(ConnectionTimeout, IsWriteTimedOutReturnsFalseWhenWithinTimeout) {
  Connection conn;
  conn.startWritePhase();  // Sets write_start to now

  EXPECT_FALSE(conn.isWriteTimedOut(30));
  EXPECT_FALSE(conn.isWriteTimedOut(60));
  EXPECT_FALSE(conn.isWriteTimedOut(1));
}

TEST(ConnectionTimeout, IsWriteTimedOutReturnsTrueWhenExpired) {
  Connection conn;
  conn.write_start = time(NULL) - 100;  // 100 seconds ago

  EXPECT_TRUE(conn.isWriteTimedOut(30));    // 30 sec timeout -> expired
  EXPECT_TRUE(conn.isWriteTimedOut(60));    // 60 sec timeout -> expired
  EXPECT_TRUE(conn.isWriteTimedOut(99));    // 99 sec timeout -> expired
  EXPECT_FALSE(conn.isWriteTimedOut(101));  // 101 sec timeout -> not expired
}

TEST(ConnectionTimeout, IsWriteTimedOutEdgeCaseExactlyAtTimeout) {
  Connection conn;
  conn.write_start = time(NULL) - 30;  // Exactly 30 seconds ago

  // At exactly timeout boundary, should be considered timed out
  EXPECT_TRUE(conn.isWriteTimedOut(30));
}

// =============================================================================
// Test: Different connection states and timeout behavior
// =============================================================================

TEST(ConnectionTimeout, EmptyBufferConnectionShouldReadTimeout) {
  Connection conn;
  conn.read_buffer = "";
  conn.read_start = time(NULL) - 60;

  EXPECT_TRUE(conn.isReadTimedOut(30));
}

TEST(ConnectionTimeout, PartialRequestLineConnectionShouldReadTimeout) {
  Connection conn;
  conn.read_buffer = "GET /";
  conn.read_start = time(NULL) - 60;

  EXPECT_TRUE(conn.isReadTimedOut(30));
}

TEST(ConnectionTimeout, PartialHeadersConnectionShouldReadTimeout) {
  Connection conn;
  conn.read_buffer = "GET / HTTP/1.1\r\nHost: localhost";
  conn.read_start = time(NULL) - 60;

  EXPECT_TRUE(conn.isReadTimedOut(30));
}

TEST(ConnectionTimeout, PartialBodyConnectionShouldReadTimeout) {
  Connection conn;
  conn.read_buffer =
      "POST /upload HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 100\r\n"
      "\r\n"
      "partial";
  conn.read_start = time(NULL) - 60;

  EXPECT_TRUE(conn.isReadTimedOut(30));
}

TEST(ConnectionTimeout, ActiveConnectionShouldNotReadTimeout) {
  Connection conn;
  conn.read_buffer = "GET /";
  // read_start is set to now on construction

  EXPECT_FALSE(conn.isReadTimedOut(30));
}

TEST(ConnectionTimeout, ConnectionWithLargeBufferShouldStillReadTimeout) {
  Connection conn;
  conn.read_buffer = std::string(10000, 'A');  // 10KB of data
  conn.read_start = time(NULL) - 60;

  EXPECT_TRUE(conn.isReadTimedOut(30));
}

// =============================================================================
// Test: Write phase timeout scenarios
// =============================================================================

TEST(ConnectionTimeout, WritePhaseNotStartedShouldNotWriteTimeout) {
  Connection conn;
  conn.read_start = time(NULL) - 100;  // Old read

  // Even though connection is old, write phase hasn't started
  EXPECT_FALSE(conn.isWriteTimedOut(30));
}

TEST(ConnectionTimeout, WritePhaseStartedShouldWriteTimeout) {
  Connection conn;
  conn.write_start = time(NULL) - 60;  // Started 60 seconds ago

  EXPECT_TRUE(conn.isWriteTimedOut(30));
}

TEST(ConnectionTimeout, SlowResponseShouldWriteTimeout) {
  Connection conn;
  conn.write_buffer = "HTTP/1.1 200 OK\r\n...";
  conn.write_start = time(NULL) - 60;

  EXPECT_TRUE(conn.isWriteTimedOut(30));
}

// =============================================================================
// Test: HTTP 408 Request Timeout status code
// =============================================================================

TEST(ConnectionTimeout, Http408StatusCodeExists) {
  EXPECT_EQ(static_cast<int>(http::S_408_REQUEST_TIMEOUT), 408);
}

TEST(ConnectionTimeout, Http408StatusTextIsCorrect) {
  std::string text = http::reasonPhrase(http::S_408_REQUEST_TIMEOUT);
  EXPECT_EQ(text, "Request Timeout");
}

TEST(ConnectionTimeout, Http408IsClientError) {
  EXPECT_TRUE(http::isClientError(http::S_408_REQUEST_TIMEOUT));
  EXPECT_FALSE(http::isServerError(http::S_408_REQUEST_TIMEOUT));
  EXPECT_FALSE(http::isSuccess(http::S_408_REQUEST_TIMEOUT));
}

TEST(ConnectionTimeout, Http408IntToStatusConversion) {
  http::Status status = http::intToStatus(408);
  EXPECT_EQ(status, http::S_408_REQUEST_TIMEOUT);
}

// =============================================================================
// Test: Default timeout constants
// =============================================================================

TEST(ConnectionTimeout, ReadTimeoutConstantExists) {
  EXPECT_GT(READ_TIMEOUT_SECONDS, 0);
  EXPECT_LE(READ_TIMEOUT_SECONDS, 300);  // Max 5 minutes reasonable
}

TEST(ConnectionTimeout, WriteTimeoutConstantExists) {
  EXPECT_GT(WRITE_TIMEOUT_SECONDS, 0);
  EXPECT_LE(WRITE_TIMEOUT_SECONDS, 300);  // Max 5 minutes reasonable
}

TEST(ConnectionTimeout, ReadTimeoutIsReasonableValue) {
  // Typical HTTP server timeout is 30-60 seconds
  EXPECT_GE(READ_TIMEOUT_SECONDS, 10);
  EXPECT_LE(READ_TIMEOUT_SECONDS, 120);
}

TEST(ConnectionTimeout, WriteTimeoutIsReasonableValue) {
  // Typical HTTP server timeout is 30-60 seconds
  EXPECT_GE(WRITE_TIMEOUT_SECONDS, 10);
  EXPECT_LE(WRITE_TIMEOUT_SECONDS, 120);
}

// =============================================================================
// Test: Timeout scenarios for real-world usage
// =============================================================================

TEST(ConnectionTimeout, SlowLorisAttackScenario) {
  // Simulates Slow Loris attack: client sends data very slowly
  // With fixed timeout, attack is mitigated
  Connection conn;
  conn.read_buffer = "GET / HTTP/1.1\r\n";  // Partial request
  conn.read_start = time(NULL) - (READ_TIMEOUT_SECONDS + 10);

  // Fixed timeout means connection will be closed regardless of partial data
  EXPECT_TRUE(conn.isReadTimedOut(READ_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, SlowLorisCannotResetTimer) {
  // Unlike updateActivity(), read_start is fixed
  Connection conn;
  conn.read_start = time(NULL) - 100;

  EXPECT_TRUE(conn.isReadTimedOut(30));

  // There's no way to reset the timer - this is intentional for security
  // The only field is read_start which is set once at connection creation
}

TEST(ConnectionTimeout, LargeFileUploadMustCompleteWithinTimeout) {
  // Client uploading large file must complete within timeout
  Connection conn;
  conn.read_buffer =
      "POST /upload HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 1000000\r\n"  // 1MB expected
      "\r\n" +
      std::string(5000, 'X');  // Only 5KB received

  // Even if still receiving data, timeout is fixed
  conn.read_start = time(NULL) - (READ_TIMEOUT_SECONDS + 1);
  EXPECT_TRUE(conn.isReadTimedOut(READ_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, MultipleConnectionsIndependentTimeouts) {
  // Each connection tracks its own timeout independently
  Connection conn1, conn2, conn3;

  conn1.read_start = time(NULL);       // Just connected
  conn2.read_start = time(NULL) - 20;  // 20 seconds old
  conn3.read_start = time(NULL) - 50;  // 50 seconds old

  EXPECT_FALSE(conn1.isReadTimedOut(30));
  EXPECT_FALSE(conn2.isReadTimedOut(30));
  EXPECT_TRUE(conn3.isReadTimedOut(30));
}

TEST(ConnectionTimeout, ReadAndWriteTimeoutsAreIndependent) {
  Connection conn;
  conn.read_start = time(NULL) - 100;  // Read phase was 100s ago
  conn.write_start = time(NULL) - 10;  // Write started 10s ago

  // Read timeout expired, but write hasn't
  EXPECT_TRUE(conn.isReadTimedOut(30));
  EXPECT_FALSE(conn.isWriteTimedOut(30));
}

TEST(ConnectionTimeout, TimeoutCheckIsIdempotent) {
  Connection conn;
  conn.read_start = time(NULL) - 60;

  // Multiple checks should return same result
  EXPECT_TRUE(conn.isReadTimedOut(30));
  EXPECT_TRUE(conn.isReadTimedOut(30));
  EXPECT_TRUE(conn.isReadTimedOut(30));

  // isReadTimedOut should not modify read_start
  EXPECT_EQ(conn.read_start, time(NULL) - 60);
}

TEST(ConnectionTimeout, TypicalRequestResponseFlow) {
  // Simulate normal request-response flow
  Connection conn;

  // Phase 1: Reading request (read_start set on construction)
  EXPECT_FALSE(conn.isReadTimedOut(READ_TIMEOUT_SECONDS));
  EXPECT_EQ(conn.write_start, 0);

  // Phase 2: Start writing response
  conn.startWritePhase();
  EXPECT_GT(conn.write_start, 0);
  EXPECT_FALSE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));

  // Both timeouts should be checked based on their respective start times
}

// =============================================================================
// Test: Clock skew protection (time going backwards)
// =============================================================================

TEST(ConnectionTimeout, IsReadTimedOutHandlesClockSkewGracefully) {
  Connection conn;
  // Simulate clock going backwards (e.g., NTP adjustment)
  conn.read_start = time(NULL) + 100;  // Set to 100 seconds in the future

  // Should not timeout (and not overflow) when clock is ahead
  EXPECT_FALSE(conn.isReadTimedOut(30));
  EXPECT_FALSE(conn.isReadTimedOut(60));
}

TEST(ConnectionTimeout, IsWriteTimedOutHandlesClockSkewGracefully) {
  Connection conn;
  conn.startWritePhase();

  // Simulate clock going backwards
  conn.write_start = time(NULL) + 100;  // Set to 100 seconds in the future

  // Should not timeout when clock is ahead
  EXPECT_FALSE(conn.isWriteTimedOut(30));
  EXPECT_FALSE(conn.isWriteTimedOut(60));
}

TEST(ConnectionTimeout, ClockSkewDoesNotCauseImmediateTimeout) {
  Connection conn;

  // Set timestamps to future (simulating clock adjustment backwards)
  conn.read_start = time(NULL) + 1000;
  conn.write_start = time(NULL) + 1000;

  // Should not report timeout despite large time difference
  EXPECT_FALSE(conn.isReadTimedOut(1));
  EXPECT_FALSE(conn.isWriteTimedOut(1));
}

TEST(ConnectionTimeout, ClockSkewProtectionIsConservative) {
  Connection conn;

  // When clock goes backwards, we choose to NOT timeout
  // This is safer than potentially closing valid connections
  conn.read_start = time(NULL) + 50;

  EXPECT_FALSE(conn.isReadTimedOut(0));    // Even with 0 timeout
  EXPECT_FALSE(conn.isReadTimedOut(100));  // Or any positive timeout
}

// =============================================================================
// Test: Write timeout scenarios for ServerManager integration
// =============================================================================

TEST(ConnectionTimeout, WriteTimeoutWithPartialResponseShouldBeDetected) {
  Connection conn;
  conn.startWritePhase();              // Start write phase
  conn.write_start = time(NULL) - 60;  // 60 seconds ago
  conn.write_buffer = "HTTP/1.1 200 OK\r\nContent-Length: 1000\r\n\r\n";
  conn.write_buffer += std::string(500, 'A');  // Only 500 of 1000 bytes written

  // Should detect write timeout even though write phase is active
  EXPECT_TRUE(conn.isWriteTimedOut(30));
}

TEST(ConnectionTimeout, WriteTimeoutWithLargeFileTransferShouldBeDetected) {
  Connection conn;
  conn.startWritePhase();
  conn.write_start = time(NULL) - 120;                // 2 minutes ago
  conn.write_buffer = std::string(1024 * 1024, 'B');  // 1MB buffer

  // Large file transfers that stall should timeout
  EXPECT_TRUE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, WriteTimeoutWithSlowClientShouldBeDetected) {
  Connection conn;
  conn.startWritePhase();
  conn.write_start = time(NULL) - 45;  // 45 seconds ago
  conn.write_buffer = "HTTP/1.1 200 OK\r\nContent-Length: 50000\r\n\r\n";
  conn.write_buffer +=
      std::string(10000, 'C');  // Slow client only received 10K of 50K

  // Slow clients that can't keep up should timeout
  EXPECT_TRUE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, WriteTimeoutWithCgiResponseShouldBeDetected) {
  Connection conn;
  conn.startWritePhase();
  conn.write_start = time(NULL) - 90;  // 90 seconds ago
  conn.write_buffer = "HTTP/1.1 200 OK\r\nContent-Length: 2048\r\n\r\n";
  conn.write_buffer += std::string(1024, 'D');  // CGI response stalled halfway

  // CGI responses that stall should timeout
  EXPECT_TRUE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, WriteTimeoutWithChunkedEncodingShouldBeDetected) {
  Connection conn;
  conn.startWritePhase();
  conn.write_start = time(NULL) - 75;  // 75 seconds ago
  conn.write_buffer = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
  conn.write_buffer += "100\r\n";              // Chunk header
  conn.write_buffer += std::string(100, 'E');  // Partial chunk data

  // Chunked encoding that stalls should timeout
  EXPECT_TRUE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, WritePhaseNotStartedShouldNotTimeout) {
  Connection conn;
  // write_start is 0, write phase never started
  conn.write_buffer = "Some data";

  // Should not timeout if write phase hasn't started
  EXPECT_FALSE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, RecentWriteActivityShouldNotTimeout) {
  Connection conn;
  conn.startWritePhase();  // Just started
  conn.write_buffer = "HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n";
  conn.write_buffer += std::string(50, 'F');

  // Recent write activity should not timeout
  EXPECT_FALSE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, WriteTimeoutBoundaryConditions) {
  Connection conn;
  conn.startWritePhase();

  // Test exact boundary - should timeout at exactly WRITE_TIMEOUT_SECONDS
  conn.write_start = time(NULL) - WRITE_TIMEOUT_SECONDS;
  EXPECT_TRUE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));

  // Test just before boundary - should not timeout
  conn.write_start = time(NULL) - (WRITE_TIMEOUT_SECONDS - 1);
  EXPECT_FALSE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));

  // Test just after boundary - should timeout
  conn.write_start = time(NULL) - (WRITE_TIMEOUT_SECONDS + 1);
  EXPECT_TRUE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, WriteTimeoutWithDifferentTimeoutValues) {
  Connection conn;
  conn.startWritePhase();
  conn.write_start = time(NULL) - 45;  // 45 seconds ago

  // Should timeout with 30s timeout
  EXPECT_TRUE(conn.isWriteTimedOut(30));

  // Should timeout with 40s timeout
  EXPECT_TRUE(conn.isWriteTimedOut(40));

  // Should NOT timeout with 60s timeout
  EXPECT_FALSE(conn.isWriteTimedOut(60));

  // Should NOT timeout with 50s timeout
  EXPECT_FALSE(conn.isWriteTimedOut(50));
}

TEST(ConnectionTimeout, WriteTimeoutWithEmptyWriteBufferShouldStillTimeout) {
  Connection conn;
  conn.startWritePhase();
  conn.write_start = time(NULL) - 60;  // 60 seconds ago
  conn.write_buffer = "";              // Empty buffer but write phase active

  // Even with empty buffer, if write phase is active and timed out, should
  // detect
  EXPECT_TRUE(conn.isWriteTimedOut(WRITE_TIMEOUT_SECONDS));
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
