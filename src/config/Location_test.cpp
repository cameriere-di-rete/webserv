#include "Location.hpp"

#include <gtest/gtest.h>

#include "HttpMethod.hpp"
#include "HttpStatus.hpp"

TEST(LocationTests, DefaultConstructorInitializesFields) {
  Location loc;
  EXPECT_EQ(loc.path, "");
  EXPECT_TRUE(loc.allow_methods.empty());
  EXPECT_EQ(loc.redirect_code, http::S_0_UNKNOWN);
  EXPECT_EQ(loc.redirect_location, "");
  EXPECT_EQ(loc.cgi_root, "");
  EXPECT_TRUE(loc.index.empty());
  EXPECT_EQ(loc.autoindex, UNSET);
  EXPECT_EQ(loc.root, "");
  EXPECT_TRUE(loc.error_page.empty());
  EXPECT_EQ(loc.max_request_body, kMaxRequestBodyUnset);
}

TEST(LocationTests, ParameterizedConstructorSetsPath) {
  Location loc("/api");
  EXPECT_EQ(loc.path, "/api");
  EXPECT_TRUE(loc.allow_methods.empty());
  EXPECT_EQ(loc.redirect_code, http::S_0_UNKNOWN);
}

TEST(LocationTests, CopyConstructorCopiesFields) {
  Location loc1("/test");
  loc1.allow_methods.insert(http::GET);
  loc1.allow_methods.insert(http::POST);
  loc1.redirect_code = http::S_301_MOVED_PERMANENTLY;
  loc1.redirect_location = "/new-location";
  loc1.cgi_root = "/usr/lib/cgi-bin";
  loc1.index.insert("index.html");
  loc1.autoindex = ON;
  loc1.root = "/var/www";
  loc1.error_page[http::S_404_NOT_FOUND] = "/404.html";
  loc1.max_request_body = 1024;

  Location loc2(loc1);
  EXPECT_EQ(loc2.path, "/test");
  EXPECT_EQ(loc2.allow_methods.size(), 2u);
  EXPECT_NE(loc2.allow_methods.find(http::GET), loc2.allow_methods.end());
  EXPECT_NE(loc2.allow_methods.find(http::POST), loc2.allow_methods.end());
  EXPECT_EQ(loc2.redirect_code, http::S_301_MOVED_PERMANENTLY);
  EXPECT_EQ(loc2.redirect_location, "/new-location");
  EXPECT_EQ(loc2.cgi_root, "/usr/lib/cgi-bin");
  EXPECT_EQ(loc2.index.size(), 1u);
  EXPECT_NE(loc2.index.find("index.html"), loc2.index.end());
  EXPECT_EQ(loc2.autoindex, ON);
  EXPECT_EQ(loc2.root, "/var/www");
  EXPECT_EQ(loc2.error_page.size(), 1u);
  EXPECT_EQ(loc2.error_page[http::S_404_NOT_FOUND], "/404.html");
  EXPECT_EQ(loc2.max_request_body, 1024u);
}

TEST(LocationTests, AssignmentOperatorCopiesFields) {
  Location loc1("/original");
  loc1.allow_methods.insert(http::DELETE);
  loc1.cgi_root = "/cgi-bin";
  loc1.autoindex = OFF;

  Location loc2;
  loc2 = loc1;
  EXPECT_EQ(loc2.path, "/original");
  EXPECT_EQ(loc2.allow_methods.size(), 1u);
  EXPECT_NE(loc2.allow_methods.find(http::DELETE), loc2.allow_methods.end());
  EXPECT_EQ(loc2.cgi_root, "/cgi-bin");
  EXPECT_EQ(loc2.autoindex, OFF);
}

TEST(LocationTests, TristateEnum) {
  EXPECT_EQ(UNSET, -1);
  EXPECT_EQ(OFF, 0);
  EXPECT_EQ(ON, 1);
}
