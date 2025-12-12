/**
 * @file Connection_test.cpp
 * @brief Unit tests for Connection class: HTTP validation and request body limits
 *
 * These tests verify that the server correctly:
 * - Validates HTTP versions (1.0, 1.1 accepted; others rejected with 505)
 * - Rejects requests with bodies exceeding max_request_body limit (413)
 * - Prepares error responses with correct status codes and versions
 */

#include "Connection.hpp"

#include <string>

#include "HttpStatus.hpp"
#include "Location.hpp"
#include "gtest/gtest.h"
#include "utils.hpp"

// =============================================================================
// Helper: Create a Location with specific max_request_body
// =============================================================================

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

  Location loc = createLocationWithMaxBody(MAX_REQUEST_BODY_UNSET);

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
