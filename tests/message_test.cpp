#include <gtest/gtest.h>

#include "Message.hpp"
#include "Header.hpp"
#include "Response.hpp"

TEST(MessageParseHeaderLine, Valid) {
  Header h;
  EXPECT_TRUE(Message::parseHeaderLine("Content-Type: text/plain; charset=utf-8", h));
  EXPECT_EQ(h.name, "Content-Type");
  EXPECT_EQ(h.value, "text/plain; charset=utf-8");
}

TEST(MessageParseHeaderLine, Invalid) {
  Header h;
  EXPECT_FALSE(Message::parseHeaderLine("NoColonHeader", h));
}

TEST(SerializeHeaders, Multiple) {
  Response r;
  r.addHeader("X-A", "1");
  r.addHeader("X-B", "2");
  std::string s = r.serializeHeaders();
  EXPECT_NE(s.find("X-A: 1"), std::string::npos);
  EXPECT_NE(s.find("X-B: 2"), std::string::npos);
}
