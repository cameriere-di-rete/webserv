#include "FileHandler.hpp"

#include <gtest/gtest.h>

TEST(FileHandlerTests, ConstructorAcceptsPath) {
  FileHandler handler("/tmp/test.txt");
  EXPECT_EQ(handler.getMonitorFd(), -1);
}

TEST(FileHandlerTests, GetMonitorFdReturnsMinusOne) {
  FileHandler handler("/var/www/index.html");
  EXPECT_EQ(handler.getMonitorFd(), -1);
}
