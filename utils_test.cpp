#include <gtest/gtest.h>
#include "utils.hpp"

#include <unistd.h>   // pipe, close
#include <fcntl.h>    // fcntl, F_GETFL, O_NONBLOCK
#include <string>
#include <set>

TEST(SetNonblockingTests, InvalidFdReturnsMinusOne) {
    EXPECT_EQ(set_nonblocking(-1), -1);
}

TEST(SetNonblockingTests, SetsNonblockingOnPipeFdAndIsIdempotent) {
    int fds[2];
    ASSERT_EQ(pipe(fds), 0) << "pipe() failed";
    int rd = fds[0];
    int wr = fds[1];

    int flags_before = fcntl(rd, F_GETFL, 0);
    ASSERT_GE(flags_before, 0) << "F_GETFL failed on pipe fd";

    int ret = set_nonblocking(rd);
    EXPECT_GE(ret, 0) << "set_nonblocking failed";

    int flags_after = fcntl(rd, F_GETFL, 0);
    ASSERT_GE(flags_after, 0);
    EXPECT_TRUE((flags_after & O_NONBLOCK) != 0) << "O_NONBLOCK not set after call";

    int ret2 = set_nonblocking(rd);
    EXPECT_GE(ret2, 0);

    close(rd);
    close(wr);
}

TEST(TrimCopyTests, EmptyString) {
    EXPECT_EQ(trim_copy(""), "");
}

TEST(TrimCopyTests, AllWhitespaceBecomesEmpty) {
    EXPECT_EQ(trim_copy("    \t\n  \r "), "");
}

TEST(TrimCopyTests, LeadingWhitespaceRemoved) {
    EXPECT_EQ(trim_copy("   hello"), "hello");
}

TEST(TrimCopyTests, TrailingWhitespaceRemoved) {
    EXPECT_EQ(trim_copy("world   \n\t"), "world");
}

TEST(TrimCopyTests, BothEndsTrimmedAndInternalPreserved) {
    EXPECT_EQ(trim_copy("  hello   world  "), "hello   world");
}

TEST(TrimCopyTests, MixedWhitespaceCharactersTrimmed) {
    std::string s = "\t\n  example\t\n";
    EXPECT_EQ(trim_copy(s), "example");
}

TEST(InitDefaultHttpMethodsTests, InsertsFiveStandardMethods) {
    std::set<http::Method> methods;
    initDefaultHttpMethods(methods);

    EXPECT_EQ(methods.size(), 5u);

    EXPECT_NE(methods.find(http::GET), methods.end());
    EXPECT_NE(methods.find(http::POST), methods.end());
    EXPECT_NE(methods.find(http::PUT), methods.end());
    EXPECT_NE(methods.find(http::DELETE), methods.end());
    EXPECT_NE(methods.find(http::HEAD), methods.end());
}
