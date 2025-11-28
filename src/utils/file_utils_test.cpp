#include "file_utils.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <string>

TEST(GuessMimeTests, CommonExtensions) {
  using namespace file_utils;
  EXPECT_EQ(guessMime("index.html"), "text/html; charset=utf-8");
  EXPECT_EQ(guessMime("readme.txt"), "text/plain; charset=utf-8");
  EXPECT_EQ(guessMime("style.css"), "text/css");
  EXPECT_EQ(guessMime("script.js"), "application/javascript");
  EXPECT_EQ(guessMime("image.jpg"), "image/jpeg");
  EXPECT_EQ(guessMime("unknown.bin"), "application/octet-stream");
}

TEST(ParseRangeTests, StartEndRange) {
  using namespace file_utils;
  off_t s, e;
  EXPECT_TRUE(parseRange("bytes=0-4", 10, s, e));
  EXPECT_EQ(s, 0);
  EXPECT_EQ(e, 4);
}

TEST(ParseRangeTests, StartOnly) {
  using namespace file_utils;
  off_t s, e;
  EXPECT_TRUE(parseRange("bytes=5-", 10, s, e));
  EXPECT_EQ(s, 5);
  EXPECT_EQ(e, 9);
}

TEST(ParseRangeTests, SuffixRange) {
  using namespace file_utils;
  off_t s, e;
  EXPECT_TRUE(parseRange("bytes=-3", 10, s, e));
  EXPECT_EQ(s, 7);
  EXPECT_EQ(e, 9);
}

TEST(PrepareFileResponseTests, NoRange) {
  using namespace file_utils;
  // create a temporary file
  char tmpl[] = "/tmp/webserv_test_XXXXXX";
  int fd = mkstemp(tmpl);
  ASSERT_GE(fd, 0);
  const char* content = "hello world";
  ssize_t w = write(fd, content, strlen(content));
  EXPECT_EQ(w, (ssize_t)strlen(content));
  fsync(fd);
  close(fd);

  std::string path(tmpl);
  ::Response resp;
  FileInfo fi;
  off_t s, e;
  int ret = prepareFileResponse(path, nullptr, resp, fi, s, e);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(resp.status_line.status_code, http::S_200_OK);

  std::string clen;
  EXPECT_TRUE(resp.getHeader("Content-Length", clen));
  EXPECT_EQ(std::stoll(clen), fi.size);

  // cleanup
  file_utils::closeFile(fi);
  unlink(path.c_str());
}
