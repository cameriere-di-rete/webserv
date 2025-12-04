#pragma once

#include <string>

namespace http {

/**
 * A class to parse and manipulate HTTP URLs/URIs.
 *
 * Supports parsing URLs with the following components:
 * - scheme (e.g., "http", "https")
 * - host (e.g., "example.com")
 * - port (e.g., 8080)
 * - path (e.g., "/path/to/resource")
 * - query (e.g., "key=value&foo=bar")
 * - fragment (e.g., "section1")
 *
 * Also handles URL encoding/decoding and path traversal detection.
 */
class Url {
 public:
  Url();
  explicit Url(const std::string& url);
  Url(const Url& other);
  Url& operator=(const Url& other);
  ~Url();

  /**
   * Parse a URL string into components.
   * @param url The URL string to parse
   * @return true if parsing succeeded, false otherwise
   */
  bool parse(const std::string& url);

  /**
   * Serialize the URL back to a string.
   * @return The serialized URL string
   */
  std::string serialize() const;

  /**
   * Get the path component without the query string.
   * @return The path component
   */
  std::string getPath() const;

  /**
   * Get the query string (without the leading '?').
   * @return The query string
   */
  std::string getQuery() const;

  /**
   * Get the fragment (without the leading '#').
   * @return The fragment
   */
  std::string getFragment() const;

  /**
   * Get the decoded path (URL-decoded).
   * @return The URL-decoded path
   */
  std::string getDecodedPath() const;

  /**
   * Check if the path contains path traversal sequences.
   * This checks the decoded path for ".." sequences.
   * @return true if path traversal is detected, false otherwise
   */
  bool hasPathTraversal() const;

  /**
   * Check if the URL is valid (was successfully parsed).
   * @return true if valid, false otherwise
   */
  bool isValid() const;

  /**
   * URL-decode a string (percent-decoding).
   * @param str The string to decode
   * @return The decoded string
   */
  static std::string decode(const std::string& str);

  /**
   * URL-encode a string (percent-encoding).
   * @param str The string to encode
   * @return The encoded string
   */
  static std::string encode(const std::string& str);

  /**
   * Normalize a path by resolving "." and ".." components.
   * @param path The path to normalize
   * @return The normalized path
   */
  static std::string normalizePath(const std::string& path);

  // Accessors for URL components
  std::string getScheme() const;
  std::string getHost() const;
  int getPort() const;

 private:
  std::string scheme_;
  std::string host_;
  int port_;
  std::string path_;
  std::string query_;
  std::string fragment_;
  bool valid_;

  /**
   * Convert a hex character to its integer value.
   * @param c The hex character ('0'-'9', 'A'-'F', 'a'-'f')
   * @return The integer value (0-15), or -1 if invalid
   */
  static int hexToInt(char c);

  /**
   * Convert an integer to a hex character.
   * @param n The integer value (0-15)
   * @return The hex character ('0'-'9', 'A'-'F')
   */
  static char intToHex(int n);
};

}  // namespace http
