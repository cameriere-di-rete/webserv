/**
 * @file ConnectionTimeout_test.cpp
 * @brief Unit tests for client request timeout handling
 *
 * These tests verify that the Connection class properly tracks separate
 * read and write timeouts for connection management.
 */

#include <cstring>
#include <ctime>
#include <string>

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "constants.hpp"
#include "gtest/gtest.h"

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
