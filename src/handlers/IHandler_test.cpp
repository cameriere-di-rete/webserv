#include "IHandler.hpp"

#include <gtest/gtest.h>

// Concrete test handler for testing IHandler interface
class TestHandler : public IHandler {
 public:
  TestHandler() : start_called(false), resume_called(false) {}

  virtual HandlerResult start(Connection&) {
    start_called = true;
    return HR_DONE;
  }

  virtual HandlerResult resume(Connection&) {
    resume_called = true;
    return HR_DONE;
  }

  bool start_called;
  bool resume_called;
};

TEST(IHandlerTests, GetMonitorFdReturnsMinusOne) {
  TestHandler handler;
  EXPECT_EQ(handler.getMonitorFd(), -1);
}

TEST(IHandlerTests, HandlerResultEnumValues) {
  EXPECT_EQ(HR_DONE, 0);
  EXPECT_EQ(HR_WOULD_BLOCK, 1);
  EXPECT_EQ(HR_ERROR, -1);
}
