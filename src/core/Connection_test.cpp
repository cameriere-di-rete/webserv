/**
 * @file Connection_test.cpp
 * @brief Unit tests for max_request_body validation during request processing
 *
 * These tests verify that the server correctly rejects requests with bodies
 * exceeding the configured max_request_body limit with HTTP 413 Payload Too
 * Large.
 */

#include <string>

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "Location.hpp"
#include "gtest/gtest.h"

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
// Test: Request body exceeds max_request_body should return 413
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
