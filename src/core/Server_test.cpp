#include "Server.hpp"

#include <gtest/gtest.h>

TEST(ServerTests, DefaultConstructorInitializesFields) {
  Server s;
  EXPECT_EQ(s.fd, -1);
  EXPECT_EQ(s.port, -1);  // Default port is -1, not 8080
  EXPECT_FALSE(s.autoindex);
  EXPECT_EQ(s.root, "");
}

TEST(ServerTests, ParameterizedConstructorSetsPort) {
  Server s(9090);
  EXPECT_EQ(s.port, 9090);
  EXPECT_EQ(s.fd, -1);
}

TEST(ServerTests, CopyConstructorCopiesFields) {
  Server s1(3000);
  s1.root = "/var/www";
  s1.autoindex = true;
  
  Server s2(s1);
  EXPECT_EQ(s2.port, 3000);
  EXPECT_EQ(s2.root, "/var/www");
  EXPECT_TRUE(s2.autoindex);
}

TEST(ServerTests, AssignmentOperatorCopiesFields) {
  Server s1(4000);
  s1.root = "/usr/share";
  s1.autoindex = false;
  
  Server s2;
  s2 = s1;
  EXPECT_EQ(s2.port, 4000);
  EXPECT_EQ(s2.root, "/usr/share");
  EXPECT_FALSE(s2.autoindex);
}

TEST(ServerTests, LocationsMapIsEmpty) {
  Server s;
  EXPECT_TRUE(s.locations.empty());
}

TEST(ServerTests, AllowMethodsSetHasDefaults) {
  Server s;
  // Server initializes with default HTTP methods (GET, POST, DELETE)
  EXPECT_FALSE(s.allow_methods.empty());
}

TEST(ServerTests, IndexSetIsEmpty) {
  Server s;
  EXPECT_TRUE(s.index.empty());
}

TEST(ServerTests, ErrorPageMapIsEmpty) {
  Server s;
  EXPECT_TRUE(s.error_page.empty());
}
