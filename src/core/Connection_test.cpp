#include "Connection.hpp"

#include <gtest/gtest.h>

#include "HttpStatus.hpp"
#include "Location.hpp"
#include "utils.hpp"

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
