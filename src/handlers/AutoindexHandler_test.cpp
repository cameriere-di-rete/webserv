#include "AutoindexHandler.hpp"

#include <gtest/gtest.h>

TEST(AutoindexHandlerTests, ConstructorAcceptsPaths) {
  AutoindexHandler handler("/var/www/html", "/");
  EXPECT_EQ(handler.getMonitorFd(), -1);
}

TEST(AutoindexHandlerTests, GetMonitorFdReturnsMinusOne) {
  AutoindexHandler handler("/tmp", "/files/");
  EXPECT_EQ(handler.getMonitorFd(), -1);
}
