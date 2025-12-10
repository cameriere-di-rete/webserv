#include "Connection.hpp"

#include <gtest/gtest.h>

TEST(ConnectionTests, DefaultConstructorInitializesFields) {
  Connection c;
  EXPECT_EQ(c.fd, -1);
  EXPECT_EQ(c.server_fd, -1);
  EXPECT_TRUE(c.read_buffer.empty());
  EXPECT_TRUE(c.write_buffer.empty());
  EXPECT_EQ(c.write_offset, 0u);
  EXPECT_EQ(c.headers_end_pos, std::string::npos);  // Initialized to npos
  EXPECT_FALSE(c.write_ready);
  EXPECT_EQ(c.active_handler, static_cast<IHandler*>(NULL));
}

TEST(ConnectionTests, ParameterizedConstructorSetsFd) {
  Connection c(42);
  EXPECT_EQ(c.fd, 42);
  EXPECT_EQ(c.server_fd, -1);
}

TEST(ConnectionTests, CopyConstructorCopiesFields) {
  Connection c1(10);
  c1.server_fd = 5;
  c1.read_buffer = "test data";
  c1.write_ready = true;

  Connection c2(c1);
  EXPECT_EQ(c2.fd, 10);
  EXPECT_EQ(c2.server_fd, 5);
  EXPECT_EQ(c2.read_buffer, "test data");
  EXPECT_TRUE(c2.write_ready);
}

TEST(ConnectionTests, AssignmentOperatorCopiesFields) {
  Connection c1(20);
  c1.server_fd = 15;
  c1.write_buffer = "response";
  c1.write_offset = 5;

  Connection c2;
  c2 = c1;
  EXPECT_EQ(c2.fd, 20);
  EXPECT_EQ(c2.server_fd, 15);
  EXPECT_EQ(c2.write_buffer, "response");
  EXPECT_EQ(c2.write_offset, 5u);
}

TEST(ConnectionTests, ActiveHandlerIsNull) {
  Connection c;
  EXPECT_EQ(c.active_handler, static_cast<IHandler*>(NULL));
}
