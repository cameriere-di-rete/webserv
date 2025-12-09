/**
 * @file ConnectionTimeout_test.cpp
 * @brief Unit tests for client request timeout handling
 *
 * These tests verify that the Connection class properly tracks activity
 * and can detect when a connection has timed out.
 */

#include <cstring>
#include <ctime>
#include <string>

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "constants.hpp"
#include "gtest/gtest.h"

// =============================================================================
// Test: Connection should have last_activity timestamp
// =============================================================================

TEST(ConnectionTimeout, ConnectionShouldHaveLastActivityField) {
  Connection conn;
  // Connection should have a last_activity field of type time_t
  EXPECT_TRUE((std::is_same<decltype(conn.last_activity), time_t>::value));
}

TEST(ConnectionTimeout, ConnectionShouldInitializeLastActivityOnConstruction) {
  time_t before = time(NULL);
  Connection conn;
  time_t after = time(NULL);

  // last_activity should be set to current time on construction
  EXPECT_GE(conn.last_activity, before);
  EXPECT_LE(conn.last_activity, after);
}

TEST(ConnectionTimeout, ConnectionFdConstructorShouldInitializeLastActivity) {
  time_t before = time(NULL);
  Connection conn(42);  // fd constructor
  time_t after = time(NULL);

  EXPECT_GE(conn.last_activity, before);
  EXPECT_LE(conn.last_activity, after);
}

TEST(ConnectionTimeout, CopyConstructorShouldCopyLastActivity) {
  Connection conn1;
  conn1.last_activity = 12345;

  Connection conn2(conn1);

  EXPECT_EQ(conn2.last_activity, 12345);
}

TEST(ConnectionTimeout, AssignmentOperatorShouldCopyLastActivity) {
  Connection conn1;
  conn1.last_activity = 67890;

  Connection conn2;
  conn2 = conn1;

  EXPECT_EQ(conn2.last_activity, 67890);
}

// =============================================================================
// Test: Connection should update timestamp on activity
// =============================================================================

TEST(ConnectionTimeout, UpdateActivityShouldExist) {
  Connection conn;
  conn.last_activity = 0;

  // Connection should have an updateActivity() method
  conn.updateActivity();

  EXPECT_GT(conn.last_activity, 0);
}

TEST(ConnectionTimeout, UpdateActivityShouldSetCurrentTime) {
  Connection conn;
  conn.last_activity = 0;

  time_t before = time(NULL);
  conn.updateActivity();
  time_t after = time(NULL);

  EXPECT_GE(conn.last_activity, before);
  EXPECT_LE(conn.last_activity, after);
}

TEST(ConnectionTimeout, MultipleUpdateActivityCallsShouldWork) {
  Connection conn;
  conn.last_activity = 0;

  conn.updateActivity();
  time_t first = conn.last_activity;

  conn.updateActivity();
  time_t second = conn.last_activity;

  EXPECT_GE(second, first);
}

// =============================================================================
// Test: Connection should detect if it's timed out
// =============================================================================

TEST(ConnectionTimeout, IsTimedOutShouldExist) {
  Connection conn;
  conn.last_activity = time(NULL);

  // Connection should have isTimedOut(timeout_seconds) method
  bool result = conn.isTimedOut(30);

  EXPECT_FALSE(result);  // Just created, should not be timed out
}

TEST(ConnectionTimeout, IsTimedOutReturnsFalseWhenWithinTimeout) {
  Connection conn;
  conn.last_activity = time(NULL);

  EXPECT_FALSE(conn.isTimedOut(30));
  EXPECT_FALSE(conn.isTimedOut(60));
  EXPECT_FALSE(conn.isTimedOut(1));
}

TEST(ConnectionTimeout, IsTimedOutReturnsTrueWhenExpired) {
  Connection conn;
  conn.last_activity = time(NULL) - 100;  // 100 seconds ago

  EXPECT_TRUE(conn.isTimedOut(30));    // 30 sec timeout -> expired
  EXPECT_TRUE(conn.isTimedOut(60));    // 60 sec timeout -> expired
  EXPECT_TRUE(conn.isTimedOut(99));    // 99 sec timeout -> expired
  EXPECT_FALSE(conn.isTimedOut(101));  // 101 sec timeout -> not expired
}

TEST(ConnectionTimeout, IsTimedOutEdgeCaseExactlyAtTimeout) {
  Connection conn;
  conn.last_activity = time(NULL) - 30;  // Exactly 30 seconds ago

  // At exactly timeout boundary, should be considered timed out
  EXPECT_TRUE(conn.isTimedOut(30));
}

TEST(ConnectionTimeout, IsTimedOutWithZeroTimeoutAlwaysTimesOut) {
  Connection conn;
  conn.last_activity = time(NULL);

  // Zero timeout means immediate timeout
  EXPECT_TRUE(conn.isTimedOut(0));
}

TEST(ConnectionTimeout, IsTimedOutWithVeryOldTimestamp) {
  Connection conn;
  conn.last_activity = 0;  // Unix epoch - very old

  EXPECT_TRUE(conn.isTimedOut(1));
  EXPECT_TRUE(conn.isTimedOut(86400));  // 24 hours
}

TEST(ConnectionTimeout, IsTimedOutWithNegativeTimeout) {
  Connection conn;
  conn.last_activity = time(NULL);

  // Negative timeout should always return true (invalid input)
  EXPECT_TRUE(conn.isTimedOut(-1));
}

// =============================================================================
// Test: Different connection states and timeout behavior
// =============================================================================

TEST(ConnectionTimeout, EmptyBufferConnectionShouldTimeout) {
  Connection conn;
  conn.read_buffer = "";
  conn.last_activity = time(NULL) - 60;

  EXPECT_TRUE(conn.isTimedOut(30));
}

TEST(ConnectionTimeout, PartialRequestLineConnectionShouldTimeout) {
  Connection conn;
  conn.read_buffer = "GET /";
  conn.last_activity = time(NULL) - 60;

  EXPECT_TRUE(conn.isTimedOut(30));
}

TEST(ConnectionTimeout, PartialHeadersConnectionShouldTimeout) {
  Connection conn;
  conn.read_buffer = "GET / HTTP/1.1\r\nHost: localhost";
  conn.last_activity = time(NULL) - 60;

  EXPECT_TRUE(conn.isTimedOut(30));
}

TEST(ConnectionTimeout, PartialBodyConnectionShouldTimeout) {
  Connection conn;
  conn.read_buffer =
      "POST /upload HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 100\r\n"
      "\r\n"
      "partial";
  conn.last_activity = time(NULL) - 60;

  EXPECT_TRUE(conn.isTimedOut(30));
}

TEST(ConnectionTimeout, ActiveConnectionShouldNotTimeout) {
  Connection conn;
  conn.read_buffer = "GET /";
  conn.last_activity = time(NULL);  // Just now

  EXPECT_FALSE(conn.isTimedOut(30));
}

TEST(ConnectionTimeout, ConnectionWithLargeBufferShouldStillTimeout) {
  Connection conn;
  conn.read_buffer = std::string(10000, 'A');  // 10KB of data
  conn.last_activity = time(NULL) - 60;

  EXPECT_TRUE(conn.isTimedOut(30));
}

// =============================================================================
// Test: Timeout with active handler (e.g., CGI)
// =============================================================================

TEST(ConnectionTimeout, ConnectionWithActiveHandlerShouldStillCheckTimeout) {
  Connection conn;
  conn.last_activity = time(NULL) - 120;

  EXPECT_TRUE(conn.isTimedOut(60));
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
// Test: Default timeout constant
// =============================================================================

TEST(ConnectionTimeout, DefaultTimeoutConstantExists) {
  EXPECT_GT(CONNECTION_TIMEOUT_SECONDS, 0);
  EXPECT_LE(CONNECTION_TIMEOUT_SECONDS, 300);  // Max 5 minutes reasonable
}

TEST(ConnectionTimeout, DefaultTimeoutIsReasonableValue) {
  // Typical HTTP server timeout is 30-60 seconds
  EXPECT_GE(CONNECTION_TIMEOUT_SECONDS, 10);
  EXPECT_LE(CONNECTION_TIMEOUT_SECONDS, 120);
}

// =============================================================================
// Test: Timeout scenarios for real-world usage
// =============================================================================

TEST(ConnectionTimeout, SlowClientScenario) {
  // Simulates a slow client that sends data sporadically
  Connection conn;

  // Client connects
  conn.last_activity = time(NULL);
  EXPECT_FALSE(conn.isTimedOut(CONNECTION_TIMEOUT_SECONDS));

  // Simulate 25 seconds passing (still within timeout)
  conn.last_activity = time(NULL) - 25;
  EXPECT_FALSE(conn.isTimedOut(CONNECTION_TIMEOUT_SECONDS));

  // Client sends more data, resetting timer
  conn.updateActivity();
  EXPECT_FALSE(conn.isTimedOut(CONNECTION_TIMEOUT_SECONDS));

  // Simulate timeout exceeded
  conn.last_activity = time(NULL) - (CONNECTION_TIMEOUT_SECONDS + 1);
  EXPECT_TRUE(conn.isTimedOut(CONNECTION_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, SlowLorisAttackScenario) {
  // Simulates Slow Loris attack: client sends data very slowly
  Connection conn;
  conn.read_buffer = "GET / HTTP/1.1\r\n";  // Partial request

  // Even with some data, old activity should timeout
  conn.last_activity = time(NULL) - (CONNECTION_TIMEOUT_SECONDS + 10);
  EXPECT_TRUE(conn.isTimedOut(CONNECTION_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, IdleConnectionAfterRequestScenario) {
  // Connection that was active but became idle
  Connection conn;
  conn.read_buffer = "";  // Empty after previous request was processed
  conn.write_buffer = "";

  conn.last_activity = time(NULL) - (CONNECTION_TIMEOUT_SECONDS + 5);
  EXPECT_TRUE(conn.isTimedOut(CONNECTION_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, KeepAliveConnectionScenario) {
  // Keep-alive connection waiting for next request
  Connection conn;
  conn.read_buffer = "";

  // Just after completing previous request
  conn.last_activity = time(NULL);
  EXPECT_FALSE(conn.isTimedOut(CONNECTION_TIMEOUT_SECONDS));

  // After timeout, should be closed
  conn.last_activity = time(NULL) - (CONNECTION_TIMEOUT_SECONDS + 1);
  EXPECT_TRUE(conn.isTimedOut(CONNECTION_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, LargeFileUploadPartialScenario) {
  // Client uploading large file but stalled
  Connection conn;
  conn.read_buffer =
      "POST /upload HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Content-Length: 1000000\r\n"  // 1MB expected
      "\r\n" +
      std::string(5000, 'X');  // Only 5KB received

  // Active upload
  conn.last_activity = time(NULL);
  EXPECT_FALSE(conn.isTimedOut(CONNECTION_TIMEOUT_SECONDS));

  // Stalled upload
  conn.last_activity = time(NULL) - (CONNECTION_TIMEOUT_SECONDS + 1);
  EXPECT_TRUE(conn.isTimedOut(CONNECTION_TIMEOUT_SECONDS));
}

TEST(ConnectionTimeout, MultipleConnectionsIndependentTimeouts) {
  // Each connection tracks its own timeout independently
  Connection conn1, conn2, conn3;

  conn1.last_activity = time(NULL);       // Active
  conn2.last_activity = time(NULL) - 20;  // 20 seconds old
  conn3.last_activity = time(NULL) - 50;  // 50 seconds old

  EXPECT_FALSE(conn1.isTimedOut(30));
  EXPECT_FALSE(conn2.isTimedOut(30));
  EXPECT_TRUE(conn3.isTimedOut(30));
}

TEST(ConnectionTimeout, UpdateActivityResetsTimeout) {
  Connection conn;
  conn.last_activity = time(NULL) - 100;  // Very old

  EXPECT_TRUE(conn.isTimedOut(30));

  // Activity resets the timeout
  conn.updateActivity();

  EXPECT_FALSE(conn.isTimedOut(30));
}

TEST(ConnectionTimeout, TimeoutCheckIsIdempotent) {
  Connection conn;
  conn.last_activity = time(NULL) - 60;

  // Multiple checks should return same result
  EXPECT_TRUE(conn.isTimedOut(30));
  EXPECT_TRUE(conn.isTimedOut(30));
  EXPECT_TRUE(conn.isTimedOut(30));

  // isTimedOut should not modify last_activity
  EXPECT_EQ(conn.last_activity, time(NULL) - 60);
}
