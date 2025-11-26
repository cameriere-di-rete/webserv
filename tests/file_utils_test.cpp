#include <gtest/gtest.h>

#include <fcntl.h>
#include <fstream>
#include <string>

#include "file_utils.hpp"
#include "Response.hpp"

using namespace file_utils;

// Helper to create a temporary file with given content. Returns path.
static std::string make_tmp_file(const std::string& content) {
  char tmpl[] = "/tmp/webserv_test_XXXXXX";
  int fd = mkstemp(tmpl);
  if (fd < 0) {
    throw std::runtime_error("mkstemp failed");
  }
  std::string path(tmpl);
  ssize_t w = write(fd, content.c_str(), content.size());
  (void)w;
  close(fd);
  return path;
}

TEST(FileUtilsParseRange, NormalRange) {
  off_t s, e;
  EXPECT_TRUE(parseRange(std::string("bytes=0-4"), 10, s, e));
  EXPECT_EQ(s, 0);
  EXPECT_EQ(e, 4);
}

TEST(FileUtilsParseRange, OpenEndedRange) {
  off_t s, e;
  EXPECT_TRUE(parseRange(std::string("bytes=5-"), 10, s, e));
  EXPECT_EQ(s, 5);
  EXPECT_EQ(e, 9);
}

TEST(FileUtilsParseRange, SuffixRange) {
  off_t s, e;
  EXPECT_TRUE(parseRange(std::string("bytes=-3"), 10, s, e));
  EXPECT_EQ(s, 7);
  EXPECT_EQ(e, 9);
}

TEST(FileUtilsParseRange, InvalidPrefix) {
  off_t s, e;
  EXPECT_FALSE(parseRange(std::string("items=0-4"), 10, s, e));
}

TEST(PrepareFileResponse, FullAndPartialAndInvalid) {
  std::string content = "0123456789"; // 10 bytes
  std::string path = make_tmp_file(content);

  FileInfo fi;
  Response resp;
  off_t start = 0, end = 0;

  // Full
  int r = prepareFileResponse(path, NULL, resp, fi, start, end);
  EXPECT_EQ(r, 0);
  EXPECT_EQ(start, 0);
  EXPECT_EQ(end, 9);
  EXPECT_EQ(fi.size, 10);
  EXPECT_EQ(resp.status_line.status_code, http::S_200_OK);
  std::string cl;
  EXPECT_TRUE(resp.getHeader("Content-Length", cl));
  EXPECT_EQ(cl, "10");
  // close file
  close(fi.fd);

  // Partial
  std::string range = "bytes=2-5";
  r = prepareFileResponse(path, &range, resp, fi, start, end);
  EXPECT_EQ(r, 0);
  EXPECT_EQ(start, 2);
  EXPECT_EQ(end, 5);
  EXPECT_EQ(resp.status_line.status_code, static_cast<http::Status>(206));
  std::string cr;
  EXPECT_TRUE(resp.getHeader("Content-Range", cr));
  EXPECT_NE(cr.find("bytes 2-5/10"), std::string::npos);
  close(fi.fd);

  // Invalid
  std::string bad = "bytes=999-1000";
  r = prepareFileResponse(path, &bad, resp, fi, start, end);
  EXPECT_EQ(r, -2);
  EXPECT_EQ(resp.status_line.status_code, static_cast<http::Status>(416));

  // remove file
  unlink(path.c_str());
}
