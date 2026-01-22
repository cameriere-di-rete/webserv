#include <gtest/gtest.h>

#include "HttpStatus.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "constants.hpp"

TEST(CookieTests, ParseCookieHeader) {
  Request req;
  std::string buf =
      "GET /test HTTP/1.1\r\nHost: example\r\nCookie: a=1; b=two; "
      "c=with%20space\r\n\r\n";
  std::size_t headers_pos = buf.find("\r\n\r\n");
  ASSERT_NE(headers_pos, std::string::npos);
  EXPECT_TRUE(req.parseStartAndHeaders(buf, headers_pos));

  std::string v;
  EXPECT_TRUE(req.getCookie("a", v));
  EXPECT_EQ(v, "1");

  EXPECT_TRUE(req.getCookie("b", v));
  EXPECT_EQ(v, "two");

  EXPECT_TRUE(req.getCookie("c", v));
  EXPECT_EQ(v, "with%20space");

  EXPECT_FALSE(req.getCookie("d", v));
}

TEST(CookieTests, ResponseAddCookie) {
  Response resp;
  resp.setStatus(http::S_200_OK, HTTP_VERSION);
  resp.addCookie("sess", "abc123", "Path=/; HttpOnly");

  std::vector<std::string> cookies = resp.getHeaders("Set-Cookie");
  ASSERT_EQ(cookies.size(), 1u);
  EXPECT_EQ(cookies[0], "sess=abc123; Path=/; HttpOnly");
}
