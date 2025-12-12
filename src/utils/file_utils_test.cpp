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

TEST(GuessMimeTests, AllSupportedExtensions) {
  using namespace file_utils;
  // Text types
  EXPECT_EQ(guessMime("file.htm"), "text/html; charset=utf-8");
  EXPECT_EQ(guessMime("data.csv"), "text/csv");
  // Application types
  EXPECT_EQ(guessMime("data.json"), "application/json");
  EXPECT_EQ(guessMime("config.xml"), "application/xml");
  EXPECT_EQ(guessMime("doc.pdf"), "application/pdf");
  EXPECT_EQ(guessMime("archive.zip"), "application/zip");
  // Image types
  EXPECT_EQ(guessMime("photo.jpeg"), "image/jpeg");
  EXPECT_EQ(guessMime("logo.png"), "image/png");
  EXPECT_EQ(guessMime("anim.gif"), "image/gif");
  EXPECT_EQ(guessMime("favicon.ico"), "image/x-icon");
  EXPECT_EQ(guessMime("vector.svg"), "image/svg+xml");
  EXPECT_EQ(guessMime("modern.webp"), "image/webp");
}

TEST(GuessMimeTests, NoExtension) {
  using namespace file_utils;
  EXPECT_EQ(guessMime("Makefile"), "application/octet-stream");
  EXPECT_EQ(guessMime("/path/to/file"), "application/octet-stream");
}

TEST(GuessMimeTests, PathWithMultipleDots) {
  using namespace file_utils;
  EXPECT_EQ(guessMime("file.backup.html"), "text/html; charset=utf-8");
  EXPECT_EQ(guessMime("archive.tar.gz"),
            "application/octet-stream");  // .gz not in map
}

TEST(MimeToExtensionTests, CommonMimeTypes) {
  using namespace file_utils;
  EXPECT_EQ(mimeToExtension("text/plain"), ".txt");
  EXPECT_EQ(mimeToExtension("text/html"), ".html");
  EXPECT_EQ(mimeToExtension("text/css"), ".css");
  EXPECT_EQ(mimeToExtension("application/javascript"), ".js");
  EXPECT_EQ(mimeToExtension("application/json"), ".json");
  EXPECT_EQ(mimeToExtension("application/xml"), ".xml");
  EXPECT_EQ(mimeToExtension("image/jpeg"), ".jpg");
  EXPECT_EQ(mimeToExtension("image/png"), ".png");
  EXPECT_EQ(mimeToExtension("image/gif"), ".gif");
}

TEST(MimeToExtensionTests, MimeWithCharset) {
  using namespace file_utils;
  // Should match even with charset parameter
  EXPECT_EQ(mimeToExtension("text/plain; charset=utf-8"), ".txt");
  EXPECT_EQ(mimeToExtension("text/html; charset=iso-8859-1"), ".html");
  EXPECT_EQ(mimeToExtension("application/json; charset=utf-8"), ".json");
}

TEST(MimeToExtensionTests, UnknownMimeType) {
  using namespace file_utils;
  EXPECT_EQ(mimeToExtension("application/octet-stream"), ".bin");
  EXPECT_EQ(mimeToExtension("video/mp4"), ".bin");
  EXPECT_EQ(mimeToExtension("audio/mpeg"), ".bin");
  EXPECT_EQ(mimeToExtension(""), ".bin");
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
