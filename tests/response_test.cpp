#include <gtest/gtest.h>

#include "Response.hpp"

TEST(ResponseSerialize, StartLineAndBody) {
  Response r;
  r.status_line.version = "HTTP/1.1";
  r.status_line.status_code = http::S_200_OK;
  r.status_line.reason = "OK";
  r.addHeader("Content-Type", "text/plain");
  r.getBody().data = "hello";
  r.addHeader("Content-Length", std::to_string(r.getBody().size()));
  std::string s = r.serialize();
  EXPECT_NE(s.find("HTTP/1.1 200"), std::string::npos);
  EXPECT_NE(s.find("Content-Type: text/plain"), std::string::npos);
  EXPECT_NE(s.find("hello"), std::string::npos);
}
