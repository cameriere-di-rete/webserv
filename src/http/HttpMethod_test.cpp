#include "HttpMethod.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(HttpMethodTests, MethodToStringAllMethods) {
  EXPECT_EQ(http::methodToString(http::GET), "GET");
  EXPECT_EQ(http::methodToString(http::POST), "POST");
  EXPECT_EQ(http::methodToString(http::PUT), "PUT");
  EXPECT_EQ(http::methodToString(http::DELETE), "DELETE");
  EXPECT_EQ(http::methodToString(http::HEAD), "HEAD");
}

TEST(HttpMethodTests, StringToMethodValidMethods) {
  EXPECT_EQ(http::stringToMethod("GET"), http::GET);
  EXPECT_EQ(http::stringToMethod("POST"), http::POST);
  EXPECT_EQ(http::stringToMethod("PUT"), http::PUT);
  EXPECT_EQ(http::stringToMethod("DELETE"), http::DELETE);
  EXPECT_EQ(http::stringToMethod("HEAD"), http::HEAD);
}

TEST(HttpMethodTests, StringToMethodInvalidMethodThrows) {
  EXPECT_THROW(http::stringToMethod("INVALID"), std::invalid_argument);
  EXPECT_THROW(http::stringToMethod("get"), std::invalid_argument);
  EXPECT_THROW(http::stringToMethod(""), std::invalid_argument);
}
