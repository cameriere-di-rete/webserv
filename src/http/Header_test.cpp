#include "Header.hpp"

#include <gtest/gtest.h>

TEST(HeaderTests, DefaultConstructorCreatesEmptyHeader) {
  Header h;
  EXPECT_EQ(h.name, "");
  EXPECT_EQ(h.value, "");
}

TEST(HeaderTests, ParameterizedConstructorSetsFields) {
  Header h("Content-Type", "text/html");
  EXPECT_EQ(h.name, "Content-Type");
  EXPECT_EQ(h.value, "text/html");
}

TEST(HeaderTests, CopyConstructorCopiesFields) {
  Header h1("Authorization", "Bearer token");
  Header h2(h1);
  EXPECT_EQ(h2.name, "Authorization");
  EXPECT_EQ(h2.value, "Bearer token");
}

TEST(HeaderTests, AssignmentOperatorCopiesFields) {
  Header h1("Host", "example.com");
  Header h2;
  h2 = h1;
  EXPECT_EQ(h2.name, "Host");
  EXPECT_EQ(h2.value, "example.com");
}

TEST(HeaderTests, AssignmentOperatorHandlesSelfAssignment) {
  Header h("Accept", "application/json");
  h = h;
  EXPECT_EQ(h.name, "Accept");
  EXPECT_EQ(h.value, "application/json");
}
