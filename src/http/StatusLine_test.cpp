#include "StatusLine.hpp"

#include <gtest/gtest.h>

#include "HttpStatus.hpp"
#include "constants.hpp"

TEST(StatusLineTests, DefaultConstructorSetsDefaults) {
  StatusLine sl;
  EXPECT_EQ(sl.version, HTTP_VERSION);
  EXPECT_EQ(sl.status_code, http::S_200_OK);
  EXPECT_EQ(sl.reason, "OK");
}

TEST(StatusLineTests, CopyConstructorCopiesFields) {
  StatusLine sl1;
  sl1.version = "HTTP/1.0";
  sl1.status_code = http::S_404_NOT_FOUND;
  sl1.reason = "Not Found";

  StatusLine sl2(sl1);
  EXPECT_EQ(sl2.version, "HTTP/1.0");
  EXPECT_EQ(sl2.status_code, http::S_404_NOT_FOUND);
  EXPECT_EQ(sl2.reason, "Not Found");
}

TEST(StatusLineTests, AssignmentOperatorCopiesFields) {
  StatusLine sl1;
  sl1.version = "HTTP/1.1";
  sl1.status_code = http::S_500_INTERNAL_SERVER_ERROR;
  sl1.reason = "Internal Server Error";

  StatusLine sl2;
  sl2 = sl1;
  EXPECT_EQ(sl2.version, "HTTP/1.1");
  EXPECT_EQ(sl2.status_code, http::S_500_INTERNAL_SERVER_ERROR);
  EXPECT_EQ(sl2.reason, "Internal Server Error");
}

TEST(StatusLineTests, ToStringFormatsCorrectly) {
  StatusLine sl;
  sl.version = "HTTP/1.1";
  sl.status_code = http::S_200_OK;
  sl.reason = "OK";

  EXPECT_EQ(sl.toString(), "HTTP/1.1 200 OK");
}

TEST(StatusLineTests, ParseValidStatusLine) {
  StatusLine sl;
  EXPECT_TRUE(sl.parse("HTTP/1.1 200 OK"));
  EXPECT_EQ(sl.version, "HTTP/1.1");
  EXPECT_EQ(sl.status_code, http::S_200_OK);
  EXPECT_EQ(sl.reason, "OK");
}

TEST(StatusLineTests, ParseValidStatusLineWithLongReason) {
  StatusLine sl;
  EXPECT_TRUE(sl.parse("HTTP/1.1 500 Internal Server Error"));
  EXPECT_EQ(sl.version, "HTTP/1.1");
  EXPECT_EQ(sl.status_code, http::S_500_INTERNAL_SERVER_ERROR);
  EXPECT_EQ(sl.reason, "Internal Server Error");
}

TEST(StatusLineTests, ParseStatusLineWithoutReason) {
  StatusLine sl;
  EXPECT_TRUE(sl.parse("HTTP/1.1 404"));
  EXPECT_EQ(sl.version, "HTTP/1.1");
  EXPECT_EQ(sl.status_code, http::S_404_NOT_FOUND);
  // Test expectation: The reason field retains its default "OK" value from
  // the constructor when no reason phrase is present in the input
  EXPECT_EQ(sl.reason, "OK");
}

TEST(StatusLineTests, ParseInvalidStatusLineReturnsFalse) {
  StatusLine sl;
  EXPECT_FALSE(sl.parse("HTTP/1.1"));
  EXPECT_FALSE(sl.parse("HTTP/1.1 abc"));
  EXPECT_FALSE(sl.parse("HTTP/1.1 999 Unknown"));
  EXPECT_FALSE(sl.parse(""));
}
