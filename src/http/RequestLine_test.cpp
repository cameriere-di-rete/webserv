#include "RequestLine.hpp"

#include <gtest/gtest.h>

TEST(RequestLineTests, DefaultConstructorCreatesEmptyRequestLine) {
  RequestLine rl;
  EXPECT_EQ(rl.method, "");
  EXPECT_EQ(rl.uri, "");
  EXPECT_EQ(rl.version, "");
}

TEST(RequestLineTests, CopyConstructorCopiesFields) {
  RequestLine rl1;
  rl1.method = "GET";
  rl1.uri = "/index.html";
  rl1.version = "HTTP/1.1";

  RequestLine rl2(rl1);
  EXPECT_EQ(rl2.method, "GET");
  EXPECT_EQ(rl2.uri, "/index.html");
  EXPECT_EQ(rl2.version, "HTTP/1.1");
}

TEST(RequestLineTests, AssignmentOperatorCopiesFields) {
  RequestLine rl1;
  rl1.method = "POST";
  rl1.uri = "/api/users";
  rl1.version = "HTTP/1.1";

  RequestLine rl2;
  rl2 = rl1;
  EXPECT_EQ(rl2.method, "POST");
  EXPECT_EQ(rl2.uri, "/api/users");
  EXPECT_EQ(rl2.version, "HTTP/1.1");
}

TEST(RequestLineTests, ToStringFormatsCorrectly) {
  RequestLine rl;
  rl.method = "GET";
  rl.uri = "/test";
  rl.version = "HTTP/1.1";

  EXPECT_EQ(rl.toString(), "GET /test HTTP/1.1");
}

TEST(RequestLineTests, ParseValidRequestLine) {
  RequestLine rl;
  EXPECT_TRUE(rl.parse("GET /index.html HTTP/1.1"));
  EXPECT_EQ(rl.method, "GET");
  EXPECT_EQ(rl.uri, "/index.html");
  EXPECT_EQ(rl.version, "HTTP/1.1");
}

TEST(RequestLineTests, ParseValidPostRequest) {
  RequestLine rl;
  EXPECT_TRUE(rl.parse("POST /api/data HTTP/1.0"));
  EXPECT_EQ(rl.method, "POST");
  EXPECT_EQ(rl.uri, "/api/data");
  EXPECT_EQ(rl.version, "HTTP/1.0");
}

TEST(RequestLineTests, ParseInvalidRequestLineReturnsFalse) {
  RequestLine rl;
  EXPECT_FALSE(rl.parse("GET"));
  EXPECT_FALSE(rl.parse("GET /path"));
  EXPECT_FALSE(rl.parse(""));
}
