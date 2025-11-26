#include <gtest/gtest.h>

#include "RequestLine.hpp"
#include "Request.hpp"

TEST(RequestLineParse, Valid) {
  RequestLine rl;
  EXPECT_TRUE(rl.parse("GET /index.html HTTP/1.1"));
  EXPECT_EQ(rl.method, "GET");
  EXPECT_EQ(rl.uri, "/index.html");
  EXPECT_EQ(rl.version, "HTTP/1.1");
}

TEST(RequestLineParse, Invalid) {
  RequestLine rl;
  EXPECT_FALSE(rl.parse("INVALID_LINE"));
}

TEST(RequestParseStartAndHeaders, Basic) {
  Request req;
  std::string raw = "GET /foo HTTP/1.1\r\nHost: example.com\r\nX-Test: val\r\n\r\n";
  std::size_t pos = raw.find("\r\n\r\n");
  EXPECT_NE(pos, std::string::npos);
  EXPECT_TRUE(req.parseStartAndHeaders(raw, pos));
  EXPECT_EQ(req.request_line.method, "GET");
  std::string host;
  EXPECT_TRUE(req.getHeader("Host", host));
  EXPECT_EQ(host, "example.com");
}
