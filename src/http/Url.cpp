#include "Url.hpp"

#include <cctype>
#include <sstream>
#include <vector>

namespace http {

Url::Url() : port_(-1), valid_(false) {}

Url::Url(const std::string& url) : port_(-1), valid_(false) {
  parse(url);
}

Url::Url(const Url& other)
    : scheme_(other.scheme_),
      host_(other.host_),
      port_(other.port_),
      path_(other.path_),
      query_(other.query_),
      fragment_(other.fragment_),
      valid_(other.valid_) {}

Url& Url::operator=(const Url& other) {
  if (this != &other) {
    scheme_ = other.scheme_;
    host_ = other.host_;
    port_ = other.port_;
    path_ = other.path_;
    query_ = other.query_;
    fragment_ = other.fragment_;
    valid_ = other.valid_;
  }
  return *this;
}

Url::~Url() {}

bool Url::parse(const std::string& url) {
  // Reset state
  scheme_.clear();
  host_.clear();
  port_ = -1;
  path_.clear();
  query_.clear();
  fragment_.clear();
  valid_ = false;

  if (url.empty()) {
    return false;
  }

  std::string remaining = url;
  std::size_t pos = 0;

  // Check for scheme (e.g., "http://")
  pos = remaining.find("://");
  if (pos != std::string::npos) {
    scheme_ = remaining.substr(0, pos);
    remaining = remaining.substr(pos + 3);

    // Parse host and optional port
    std::size_t path_start = remaining.find('/');
    std::string authority;
    if (path_start != std::string::npos) {
      authority = remaining.substr(0, path_start);
      remaining = remaining.substr(path_start);
    } else {
      authority = remaining;
      remaining = "/";
    }

    // Check for port in authority
    std::size_t port_pos = authority.rfind(':');
    if (port_pos != std::string::npos) {
      host_ = authority.substr(0, port_pos);
      std::string port_str = authority.substr(port_pos + 1);
      // Parse port number
      port_ = 0;
      for (std::size_t i = 0; i < port_str.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(port_str[i]))) {
          return false;  // Invalid port
        }
        port_ = port_ * 10 + (port_str[i] - '0');
      }
    } else {
      host_ = authority;
    }
  }

  // Parse path, query, and fragment from remaining string
  // Extract fragment first (after #)
  pos = remaining.find('#');
  if (pos != std::string::npos) {
    fragment_ = remaining.substr(pos + 1);
    remaining = remaining.substr(0, pos);
  }

  // Extract query string (after ?)
  pos = remaining.find('?');
  if (pos != std::string::npos) {
    query_ = remaining.substr(pos + 1);
    remaining = remaining.substr(0, pos);
  }

  // What remains is the path
  path_ = remaining;

  // A URL with at least a path is valid
  valid_ = !path_.empty();
  return valid_;
}

std::string Url::serialize() const {
  std::ostringstream oss;

  if (!scheme_.empty()) {
    oss << scheme_ << "://";
    if (!host_.empty()) {
      oss << host_;
      if (port_ > 0) {
        oss << ":" << port_;
      }
    }
  }

  oss << path_;

  if (!query_.empty()) {
    oss << "?" << query_;
  }

  if (!fragment_.empty()) {
    oss << "#" << fragment_;
  }

  return oss.str();
}

std::string Url::getPath() const {
  return path_;
}

std::string Url::getQuery() const {
  return query_;
}

std::string Url::getFragment() const {
  return fragment_;
}

std::string Url::getDecodedPath() const {
  return decode(path_);
}

bool Url::hasPathTraversal() const {
  // Decode the path first, then check for ".." sequences
  std::string decoded = getDecodedPath();

  // Check for ".." at various positions
  // 1. Exactly ".."
  if (decoded == "..") {
    return true;
  }

  // 2. Starts with "../"
  if (decoded.size() >= 3 && decoded.substr(0, 3) == "../") {
    return true;
  }

  // 3. Ends with "/.."
  if (decoded.size() >= 3 && decoded.substr(decoded.size() - 3) == "/..") {
    return true;
  }

  // 4. Contains "/../"
  if (decoded.find("/../") != std::string::npos) {
    return true;
  }

  return false;
}

bool Url::isValid() const {
  return valid_;
}

std::string Url::getScheme() const {
  return scheme_;
}

std::string Url::getHost() const {
  return host_;
}

int Url::getPort() const {
  return port_;
}

int Url::hexToInt(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  return -1;
}

char Url::intToHex(int n) {
  if (n >= 0 && n <= 9) {
    return static_cast<char>('0' + n);
  }
  if (n >= 10 && n <= 15) {
    return static_cast<char>('A' + n - 10);
  }
  return '0';
}

std::string Url::decode(const std::string& str) {
  std::string result;
  result.reserve(str.size());

  for (std::size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '%' && i + 2 < str.size()) {
      int high = hexToInt(str[i + 1]);
      int low = hexToInt(str[i + 2]);
      if (high >= 0 && low >= 0) {
        result += static_cast<char>((high << 4) | low);
        i += 2;
        continue;
      }
    }
    if (str[i] == '+') {
      result += ' ';
    } else {
      result += str[i];
    }
  }

  return result;
}

std::string Url::encode(const std::string& str) {
  std::string result;
  result.reserve(str.size() * 3);  // Worst case: all characters need encoding

  for (std::size_t i = 0; i < str.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(str[i]);

    // Unreserved characters (RFC 3986)
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      result += static_cast<char>(c);
    } else {
      result += '%';
      result += intToHex((c >> 4) & 0x0F);
      result += intToHex(c & 0x0F);
    }
  }

  return result;
}

std::string Url::normalizePath(const std::string& path) {
  if (path.empty()) {
    return "/";
  }

  // Decode the path first
  std::string decoded = decode(path);

  // Split path into segments
  std::vector<std::string> segments;
  std::string segment;
  bool absolute = (!decoded.empty() && decoded[0] == '/');

  for (std::size_t i = 0; i < decoded.size(); ++i) {
    if (decoded[i] == '/') {
      if (!segment.empty()) {
        if (segment == "..") {
          // Go up one directory if possible
          if (!segments.empty()) {
            segments.pop_back();
          }
        } else if (segment != ".") {
          segments.push_back(segment);
        }
        segment.clear();
      }
    } else {
      segment += decoded[i];
    }
  }

  // Handle trailing segment
  if (!segment.empty()) {
    if (segment == "..") {
      if (!segments.empty()) {
        segments.pop_back();
      }
    } else if (segment != ".") {
      segments.push_back(segment);
    }
  }

  // Rebuild path
  std::string result;
  if (absolute) {
    result = "/";
  }

  for (std::size_t i = 0; i < segments.size(); ++i) {
    if (i > 0) {
      result += "/";
    }
    result += segments[i];
  }

  // Preserve trailing slash if original had it
  if (result.size() > 1 && !decoded.empty() &&
      decoded[decoded.size() - 1] == '/') {
    result += "/";
  }

  if (result.empty()) {
    result = "/";
  }

  return result;
}

}  // namespace http
