#include "Url.hpp"

#include <gtest/gtest.h>

using namespace http;

// ==================== PARSING TESTS ====================

TEST(UrlParseTests, SimpleAbsolutePath) {
  Url url("/path/to/resource");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getPath(), "/path/to/resource");
  EXPECT_EQ(url.getQuery(), "");
  EXPECT_EQ(url.getFragment(), "");
}

TEST(UrlParseTests, PathWithQueryString) {
  Url url("/search?q=hello&page=1");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getPath(), "/search");
  EXPECT_EQ(url.getQuery(), "q=hello&page=1");
}

TEST(UrlParseTests, PathWithFragment) {
  Url url("/page#section1");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getPath(), "/page");
  EXPECT_EQ(url.getFragment(), "section1");
}

TEST(UrlParseTests, PathWithQueryAndFragment) {
  Url url("/page?id=5#top");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getPath(), "/page");
  EXPECT_EQ(url.getQuery(), "id=5");
  EXPECT_EQ(url.getFragment(), "top");
}

TEST(UrlParseTests, FullUrl) {
  Url url("http://example.com:8080/path?query=1#frag");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getScheme(), "http");
  EXPECT_EQ(url.getHost(), "example.com");
  EXPECT_EQ(url.getPort(), 8080);
  EXPECT_EQ(url.getPath(), "/path");
  EXPECT_EQ(url.getQuery(), "query=1");
  EXPECT_EQ(url.getFragment(), "frag");
}

TEST(UrlParseTests, UrlWithoutPort) {
  Url url("https://example.com/resource");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getScheme(), "https");
  EXPECT_EQ(url.getHost(), "example.com");
  EXPECT_EQ(url.getPort(), -1);
  EXPECT_EQ(url.getPath(), "/resource");
}

TEST(UrlParseTests, EmptyUrl) {
  Url url("");
  EXPECT_FALSE(url.isValid());
}

TEST(UrlParseTests, RootPath) {
  Url url("/");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getPath(), "/");
}

// ==================== PORT VALIDATION TESTS ====================

TEST(UrlParseTests, InvalidPortWithNonDigits) {
  Url url("http://example.com:abc/path");
  EXPECT_FALSE(url.isValid());
}

TEST(UrlParseTests, PortOverflow) {
  Url url("http://example.com:999999999999999999999/path");
  EXPECT_FALSE(url.isValid());
}

TEST(UrlParseTests, PortOutOfValidRange) {
  Url url("http://example.com:99999/path");
  EXPECT_FALSE(url.isValid());
}

TEST(UrlParseTests, ValidPortAtMaxRange) {
  Url url("http://example.com:65535/path");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getPort(), 65535);
}

TEST(UrlParseTests, ValidPortAtMinRange) {
  Url url("http://example.com:0/path");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getPort(), 0);
}

// ==================== URL DECODING TESTS ====================

TEST(UrlDecodeTests, NoEncoding) {
  EXPECT_EQ(Url::decode("hello"), "hello");
}

TEST(UrlDecodeTests, SpaceAsPlus) {
  EXPECT_EQ(Url::decode("hello+world"), "hello world");
}

TEST(UrlDecodeTests, PercentEncodedSpace) {
  EXPECT_EQ(Url::decode("hello%20world"), "hello world");
}

TEST(UrlDecodeTests, PercentEncodedDot) {
  EXPECT_EQ(Url::decode("%2e"), ".");
  EXPECT_EQ(Url::decode("%2E"), ".");
}

TEST(UrlDecodeTests, PercentEncodedDoubleDot) {
  EXPECT_EQ(Url::decode("%2e%2e"), "..");
  EXPECT_EQ(Url::decode("%2E%2E"), "..");
  EXPECT_EQ(Url::decode("%2e%2E"), "..");
  EXPECT_EQ(Url::decode("%2E%2e"), "..");
}

TEST(UrlDecodeTests, MixedEncoding) {
  EXPECT_EQ(Url::decode("/path%2Fto%2Fresource"), "/path/to/resource");
}

TEST(UrlDecodeTests, InvalidPercentSequence) {
  // Invalid hex characters - should pass through unchanged
  EXPECT_EQ(Url::decode("%GG"), "%GG");
  EXPECT_EQ(Url::decode("%2"), "%2");
}

TEST(UrlDecodeTests, SpecialCharacters) {
  EXPECT_EQ(Url::decode("%21"), "!");
  EXPECT_EQ(Url::decode("%40"), "@");
  EXPECT_EQ(Url::decode("%23"), "#");
}

// ==================== URL ENCODING TESTS ====================

TEST(UrlEncodeTests, NoEncodingNeeded) {
  EXPECT_EQ(Url::encode("hello"), "hello");
  EXPECT_EQ(Url::encode("Hello-World_123.txt"), "Hello-World_123.txt");
}

TEST(UrlEncodeTests, SpaceEncoded) {
  EXPECT_EQ(Url::encode("hello world"), "hello%20world");
}

TEST(UrlEncodeTests, SpecialCharacters) {
  EXPECT_EQ(Url::encode("a/b"), "a%2Fb");
  EXPECT_EQ(Url::encode("a?b"), "a%3Fb");
  EXPECT_EQ(Url::encode("a#b"), "a%23b");
}

TEST(UrlEncodeTests, NonAscii) {
  // UTF-8 encoded string
  EXPECT_EQ(Url::encode("\xC3\xA9"), "%C3%A9");  // é in UTF-8
}

// ==================== PATH TRAVERSAL DETECTION TESTS ====================

TEST(UrlPathTraversalTests, NoDotDot) {
  Url url("/path/to/file");
  EXPECT_FALSE(url.hasPathTraversal());
}

TEST(UrlPathTraversalTests, SimpleDotDot) {
  Url url("/path/../secret");
  EXPECT_TRUE(url.hasPathTraversal());
}

TEST(UrlPathTraversalTests, DotDotAtStart) {
  Url url("/../etc/passwd");
  EXPECT_TRUE(url.hasPathTraversal());
}

TEST(UrlPathTraversalTests, DotDotAtEnd) {
  Url url("/path/to/..");
  EXPECT_TRUE(url.hasPathTraversal());
}

TEST(UrlPathTraversalTests, EncodedDotDotLowercase) {
  Url url("/path/%2e%2e/secret");
  EXPECT_TRUE(url.hasPathTraversal());
}

TEST(UrlPathTraversalTests, EncodedDotDotUppercase) {
  Url url("/path/%2E%2E/secret");
  EXPECT_TRUE(url.hasPathTraversal());
}

TEST(UrlPathTraversalTests, EncodedDotDotMixed) {
  Url url("/path/%2e%2E/secret");
  EXPECT_TRUE(url.hasPathTraversal());
}

TEST(UrlPathTraversalTests, SingleDot) {
  Url url("/path/./file");
  EXPECT_FALSE(url.hasPathTraversal());  // Single dot is not traversal
}

TEST(UrlPathTraversalTests, TripleDot) {
  Url url("/path/.../file");
  EXPECT_FALSE(url.hasPathTraversal());  // "..." is not traversal
}

TEST(UrlPathTraversalTests, DotDotInFilename) {
  Url url("/path/file..txt");
  EXPECT_FALSE(url.hasPathTraversal());  // ".." in filename is OK
}

// ==================== PATH NORMALIZATION TESTS ====================

TEST(UrlNormalizeTests, AlreadyNormalized) {
  EXPECT_EQ(Url::normalizePath("/a/b/c"), "/a/b/c");
}

TEST(UrlNormalizeTests, SingleDots) {
  EXPECT_EQ(Url::normalizePath("/a/./b/./c"), "/a/b/c");
}

TEST(UrlNormalizeTests, DoubleDots) {
  EXPECT_EQ(Url::normalizePath("/a/b/../c"), "/a/c");
}

TEST(UrlNormalizeTests, MultipleDoubleDots) {
  EXPECT_EQ(Url::normalizePath("/a/b/c/../../d"), "/a/d");
}

TEST(UrlNormalizeTests, DoubleDotAtStart) {
  // Can't go above root
  EXPECT_EQ(Url::normalizePath("/../a"), "/a");
}

TEST(UrlNormalizeTests, EncodedPath) {
  EXPECT_EQ(Url::normalizePath("/a/%2e%2e/b"), "/b");
}

TEST(UrlNormalizeTests, EmptyPath) {
  EXPECT_EQ(Url::normalizePath(""), "/");
}

TEST(UrlNormalizeTests, RootPath) {
  EXPECT_EQ(Url::normalizePath("/"), "/");
}

// ==================== SERIALIZATION TESTS ====================

TEST(UrlSerializeTests, SimplePath) {
  Url url("/path/to/file");
  EXPECT_EQ(url.serialize(), "/path/to/file");
}

TEST(UrlSerializeTests, PathWithQuery) {
  Url url("/search?q=test");
  EXPECT_EQ(url.serialize(), "/search?q=test");
}

TEST(UrlSerializeTests, PathWithQueryAndFragment) {
  Url url("/page?id=1#top");
  EXPECT_EQ(url.serialize(), "/page?id=1#top");
}

TEST(UrlSerializeTests, FullUrl) {
  Url url("http://example.com:8080/path?q=1#f");
  EXPECT_EQ(url.serialize(), "http://example.com:8080/path?q=1#f");
}

// ==================== DECODED PATH TESTS ====================

TEST(UrlDecodedPathTests, NoEncoding) {
  Url url("/path/to/file");
  EXPECT_EQ(url.getDecodedPath(), "/path/to/file");
}

TEST(UrlDecodedPathTests, EncodedSpaces) {
  Url url("/path%20to%20file");
  EXPECT_EQ(url.getDecodedPath(), "/path to file");
}

TEST(UrlDecodedPathTests, EncodedSlash) {
  Url url("/path%2Fto%2Ffile");
  EXPECT_EQ(url.getDecodedPath(), "/path/to/file");
}

// ==================== COPY AND ASSIGNMENT TESTS ====================

TEST(UrlCopyTests, CopyConstructor) {
  Url url1("http://example.com:8080/path?q=1#f");
  Url url2(url1);
  EXPECT_EQ(url2.getScheme(), url1.getScheme());
  EXPECT_EQ(url2.getHost(), url1.getHost());
  EXPECT_EQ(url2.getPort(), url1.getPort());
  EXPECT_EQ(url2.getPath(), url1.getPath());
  EXPECT_EQ(url2.getQuery(), url1.getQuery());
  EXPECT_EQ(url2.getFragment(), url1.getFragment());
}

TEST(UrlCopyTests, AssignmentOperator) {
  Url url1("http://example.com:8080/path?q=1#f");
  Url url2;
  // Verify default-constructed state before assignment
  EXPECT_FALSE(url2.isValid());
  EXPECT_EQ(url2.getScheme(), "");
  EXPECT_EQ(url2.getHost(), "");
  EXPECT_EQ(url2.getPort(), -1);
  EXPECT_EQ(url2.getPath(), "");
  EXPECT_EQ(url2.getQuery(), "");
  EXPECT_EQ(url2.getFragment(), "");

  url2 = url1;
  EXPECT_EQ(url2.getScheme(), url1.getScheme());
  EXPECT_EQ(url2.getHost(), url1.getHost());
  EXPECT_EQ(url2.getPort(), url1.getPort());
  EXPECT_EQ(url2.getPath(), url1.getPath());
  EXPECT_EQ(url2.getQuery(), url1.getQuery());
  EXPECT_EQ(url2.getFragment(), url1.getFragment());
}
