#include "Config.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "HttpMethod.hpp"
#include "HttpStatus.hpp"
#include "Location.hpp"
#include "Logger.hpp"
#include "utils.hpp"

// ==================== PUBLIC METHODS ====================

Config::Config()
    : tokens_(),
      root_(),
      servers_(),
      global_error_pages_(),
      global_max_request_body_(kMaxRequestBodyUnset),
      idx_(0),
      current_server_index_(kGlobalContext),
      current_location_path_() {}

Config::~Config() {}

Config::Config(const Config& other)
    : tokens_(other.tokens_),
      root_(other.root_),
      servers_(other.servers_),
      global_error_pages_(other.global_error_pages_),
      global_max_request_body_(other.global_max_request_body_),
      idx_(other.idx_),
      current_server_index_(other.current_server_index_),
      current_location_path_(other.current_location_path_) {}

Config& Config::operator=(const Config& other) {
  if (this != &other) {
    tokens_ = other.tokens_;
    idx_ = other.idx_;
    root_ = other.root_;
    servers_ = other.servers_;
    global_error_pages_ = other.global_error_pages_;
    global_max_request_body_ = other.global_max_request_body_;
    current_server_index_ = other.current_server_index_;
    current_location_path_ = other.current_location_path_;
  }
  return *this;
}

void Config::parseFile(const std::string& path) {
  LOG(INFO) << "Starting to parse config file: " << path;

  // read file
  std::ifstream file(path.c_str());
  if (!file.is_open()) {
    LOG(ERROR) << "Unable to open config file: " << path;
    throw std::runtime_error(std::string("Unable to open config file: ") +
                             path);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  LOG(DEBUG) << "File content size: " << content.size() << " bytes";

  removeComments(content);
  LOG(DEBUG) << "Comments removed, tokenizing...";

  tokenize(content);
  LOG(INFO) << "Tokenization complete. Total tokens: " << tokens_.size();

  root_.type = "root";
  while (!eof()) {
    if (isBlock()) {
      LOG(DEBUG) << "Found block '" << peek() << "', parsing...";
      root_.sub_blocks.push_back(parseBlock());
    } else {
      LOG(DEBUG) << "Found global directive: " << peek();
      root_.directives.push_back(parseDirective());
    }
  }
  LOG(INFO) << "Config file parsed successfully. Server blocks found: "
            << root_.sub_blocks.size();
}

std::vector<Server> Config::getServers(void) {
  LOG(INFO) << "Validating configuration before building servers";

  // Ensure there is at least one server block
  if (root_.sub_blocks.empty()) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "No server blocks defined";
    std::string msg = oss.str();
    LOG(ERROR) << msg;
    throw std::runtime_error(msg);
  }

  // Ensure that all top-level sub-blocks are `server` blocks
  for (size_t i = 0; i < root_.sub_blocks.size(); ++i) {
    const BlockNode& b = root_.sub_blocks[i];
    if (b.type != "server") {
      std::ostringstream oss;
      oss << configErrorPrefix() << "unexpected top-level block '" << b.type
          << "' at index " << i << " (expected 'server')";
      std::string msg = oss.str();
      LOG(ERROR) << msg;
      throw std::runtime_error(msg);
    }
  }

  // Parse and validate global directives
  global_max_request_body_ = kMaxRequestBodyUnset;
  global_error_pages_.clear();

  LOG(DEBUG) << "Processing " << root_.directives.size()
             << " global directive(s)";
  for (size_t i = 0; i < root_.directives.size(); ++i) {
    const DirectiveNode& d = root_.directives[i];

    if (d.name == "error_page") {
      requireArgsAtLeast_(d, 2);
      global_error_pages_ = parseErrorPages(d.args);
      for (std::map<http::Status, std::string>::const_iterator it =
               global_error_pages_.begin();
           it != global_error_pages_.end(); ++it) {
        LOG(DEBUG) << "Global error_page: " << it->first << " -> "
                   << it->second;
      }

    } else if (d.name == "max_request_body") {
      requireArgsEqual_(d, 1);
      global_max_request_body_ = parsePositiveNumber_(d.args[0]);
      LOG(DEBUG) << "Global max_request_body set to: "
                 << global_max_request_body_;
    } else {
      throwUnrecognizedDirective_(d, "as global directive");
    }
  }

  LOG(DEBUG) << "Building server objects from configuration...";
  servers_.clear();

  for (size_t i = 0; i < root_.sub_blocks.size(); ++i) {
    const BlockNode& block = root_.sub_blocks[i];
    if (block.type == "server") {
      LOG(DEBUG) << "Translating server block #" << i;
      Server srv;
      translateServerBlock_(block, srv, i);
      servers_.push_back(srv);
      LOG(DEBUG) << "Server #" << i << " created - Port: " << srv.port
                 << ", Locations: " << srv.locations.size();
    }
  }
  LOG(DEBUG) << "Built " << servers_.size() << " server(s)";

  return servers_;
}

// ==================== ERROR HELPER ====================

// Return the appropriate configuration error prefix depending on context.
std::string Config::configErrorPrefix() const {
  std::ostringstream oss;
  if (current_server_index_ != kGlobalContext) {
    oss << "Configuration error in server #" << current_server_index_;
    if (!current_location_path_.empty()) {
      oss << " location '" << current_location_path_ << "'";
    }
  } else {
    oss << "Configuration error";
  }
  oss << ": ";
  return oss.str();
}

// Centralised helper to throw standardized unrecognized-directive errors.
// `context` will be appended after the directive message (for example:
// "in server block", "in location block", or "global directive").
void Config::throwUnrecognizedDirective_(const DirectiveNode& d,
                                         const std::string& context) const {
  std::ostringstream oss;
  oss << configErrorPrefix() << "Unrecognized directive '" << d.name << "'";
  if (!context.empty()) {
    oss << " " << context;
  }
  std::string msg = oss.str();
  LOG(ERROR) << msg;
  throw std::runtime_error(msg);
}

// ==================== DEBUG OUTPUT ====================

static void _printBlockRec(const BlockNode& b, int indent) {
  std::string pad(indent, ' ');
  {
    std::ostringstream ss;
    ss << pad << "Block: type='" << b.type << "'";
    if (!b.param.empty()) {
      ss << " param='" << b.param << "'";
    }
    LOG(DEBUG) << ss.str();
  }
  for (size_t i = 0; i < b.directives.size(); ++i) {
    const DirectiveNode& d = b.directives[i];
    std::ostringstream ss;
    ss << pad << "  Directive: name='" << d.name << "' args=[";
    for (size_t j = 0; j < d.args.size(); ++j) {
      if (j) {
        ss << ", ";
      }
      ss << "'" << d.args[j] << "'";
    }
    ss << "]";
    LOG(DEBUG) << ss.str();
  }
  for (size_t i = 0; i < b.sub_blocks.size(); ++i) {
    _printBlockRec(b.sub_blocks[i], indent + 2);
  }
}

void Config::debug(void) const {
  _printBlockRec(root_, 0);
}

// ==================== PARSING HELPERS ====================

void Config::removeComments(std::string& s) {
  size_t pos = 0;
  while ((pos = s.find('#', pos)) != std::string::npos) {
    size_t e = s.find('\n', pos);
    if (e == std::string::npos) {
      s.erase(pos);
      break;
    }
    s.erase(pos, e - pos);
  }
}

void Config::tokenize(const std::string& content) {
  tokens_.clear();
  std::string cur;
  for (size_t i = 0; i < content.size(); ++i) {
    char c = content[i];
    if (c == '{' || c == '}' || c == ';') {
      if (!cur.empty()) {
        tokens_.push_back(cur);
        cur.clear();
      }
      tokens_.push_back(std::string(1, c));
    } else if (std::isspace(static_cast<unsigned char>(c))) {
      if (!cur.empty()) {
        tokens_.push_back(cur);
        cur.clear();
      }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) {
    tokens_.push_back(cur);
  }
  idx_ = 0;
}

bool Config::eof() const {
  return idx_ >= tokens_.size();
}

const std::string& Config::peek() const {
  static std::string empty = "";
  return idx_ < tokens_.size() ? tokens_[idx_] : empty;
}

std::string Config::get() {
  if (idx_ >= tokens_.size()) {
    throw std::runtime_error("Unexpected end of tokens");
  }
  return tokens_[idx_++];
}

bool Config::isBlock() const {
  // A block is identified by a '{' following the current token.
  // This can be:
  //   - Immediately after (e.g., "server {")
  //   - After one parameter (e.g., "location /path {")
  // We look ahead at most 2 positions to accommodate blocks with parameters.
  return (idx_ + 1 < tokens_.size() && tokens_[idx_ + 1] == "{") ||
         (idx_ + 2 < tokens_.size() && tokens_[idx_ + 2] == "{");
}

DirectiveNode Config::parseDirective() {
  DirectiveNode d;
  d.name = get();
  while (peek() != ";") {
    if (eof()) {
      throw std::runtime_error(std::string("Directive '") + d.name +
                               "' missing ';'");
    }
    d.args.push_back(get());
  }
  get();  // consume ;
  return d;
}

BlockNode Config::parseBlock() {
  BlockNode b;
  b.type = get();  // server or location
  if (b.type == "location") {
    if (peek().empty()) {
      throw std::runtime_error("location missing parameter");
    }
    b.param = get();
  }
  if (get() != "{") {
    throw std::runtime_error("Expected '{' after block type");
  }
  while (peek() != "}") {
    if (eof()) {
      throw std::runtime_error(std::string("Missing '}' for block ") + b.type);
    }
    if (isBlock()) {
      b.sub_blocks.push_back(parseBlock());
    } else {
      b.directives.push_back(parseDirective());
    }
  }
  get();  // consume }
  return b;
}

// ==================== VALIDATION METHODS ====================

int Config::parsePortValue_(const std::string& portstr) {
  std::size_t n = parsePositiveNumber_(portstr);
  if (n < 1 || n > 65535) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Invalid port number " << n
        << " (must be 1-65535)";
    LOG(ERROR) << oss.str();
    throw std::runtime_error(oss.str());
  }
  return static_cast<int>(n);
}

bool Config::parseBooleanValue_(const std::string& value) {
  if (value == "on") {
    return true;
  }
  if (value == "off") {
    return false;
  }
  std::ostringstream oss;
  oss << configErrorPrefix() << "Invalid boolean value '" << value
      << "' (expected: on/off)";
  throw std::runtime_error(oss.str());
}

http::Method Config::parseHttpMethod_(const std::string& method) {
  try {
    return http::stringToMethod(method);
  } catch (const std::invalid_argument& e) {
    std::ostringstream oss;
    oss << configErrorPrefix() << e.what();
    throw std::runtime_error(oss.str());
  }
}

http::Status Config::parseRedirectCode_(const std::string& value) {
  std::size_t code_sz = parsePositiveNumber_(value);
  int code = static_cast<int>(code_sz);
  if (code_sz > static_cast<std::size_t>(INT_MAX)) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Invalid redirect status code " << code
        << " (valid: 301, 302, 303, 307, 308)";
    throw std::runtime_error(oss.str());
  }

  try {
    http::Status s = http::intToStatus(code);
    if (!http::isRedirect(s)) {
      std::ostringstream oss;
      oss << configErrorPrefix() << "Invalid redirect status code " << code
          << " (valid: 301, 302, 303, 307, 308)";
      throw std::runtime_error(oss.str());
    }
    return s;
  } catch (const std::invalid_argument&) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Invalid redirect status code " << code
        << " (valid: 301, 302, 303, 307, 308)";
    throw std::runtime_error(oss.str());
  }
}

std::size_t Config::parsePositiveNumber_(const std::string& value) {
  for (size_t i = 0; i < value.size(); ++i) {
    if (value[i] < '0' || value[i] > '9') {
      std::ostringstream oss;
      oss << configErrorPrefix() << "Invalid positive number '" << value << "'";
      throw std::runtime_error(oss.str());
    }
  }

  errno = 0;
  char* endptr = NULL;
  long num = std::strtol(value.c_str(), &endptr, 10);

  if (errno == ERANGE) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Numeric value out of range: '" << value
        << "'";
    throw std::runtime_error(oss.str());
  }
  if (endptr == NULL || *endptr != '\0') {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Invalid numeric value '" << value << "'";
    throw std::runtime_error(oss.str());
  }
  if (num <= 0) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Invalid positive number '" << value << "'";
    throw std::runtime_error(oss.str());
  }

  return static_cast<std::size_t>(num);
}

void Config::requireArgsAtLeast_(const DirectiveNode& d, size_t n) const {
  if (d.args.size() < n) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Directive '" << d.name
        << "' requires at least " << n << " argument(s)";
    std::string msg = oss.str();
    LOG(ERROR) << msg;
    throw std::runtime_error(msg);
  }
}

void Config::requireArgsEqual_(const DirectiveNode& d, size_t n) const {
  if (d.args.size() != n) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Directive '" << d.name
        << "' requires exactly " << n << " argument(s)";
    std::string msg = oss.str();
    LOG(ERROR) << msg;
    throw std::runtime_error(msg);
  }
}

// Validate a list of HTTP methods and populate the destination set with
// http::Method entries.
std::set<http::Method> Config::parseMethods(
    const std::vector<std::string>& args) {
  std::set<http::Method> dest;
  for (size_t i = 0; i < args.size(); ++i) {
    const std::string& m = args[i];
    http::Method mm = parseHttpMethod_(m);
    dest.insert(mm);
  }
  return dest;
}

// Validate and populate error_page mappings. The args vector is expected
// to have one or more status codes followed by a final path. For example:
// ["500","502","/50x.html"]
std::map<http::Status, std::string> Config::parseErrorPages(
    const std::vector<std::string>& args) {
  if (args.size() < 2) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Directive requires at least two args";
    throw std::runtime_error(oss.str());
  }
  const std::string& path = args[args.size() - 1];
  std::map<http::Status, std::string> dest;
  for (size_t i = 0; i + 1 < args.size(); ++i) {
    http::Status code = parseStatusCode_(args[i]);
    // centralised validation (now accepts enum)
    validateErrorPageCode_(code);
    dest[code] = path;
  }
  return dest;
}

void Config::validateErrorPageCode_(http::Status code) const {
  if (!(http::isClientError(code) || http::isServerError(code))) {
    throwInvalidErrorPageCode_(code);
  }
}

void Config::throwInvalidErrorPageCode_(http::Status code) const {
  std::ostringstream oss;
  oss << configErrorPrefix() << "Invalid error_page status code " << code
      << " (must be 4xx or 5xx)";
  std::string msg = oss.str();
  LOG(ERROR) << msg;
  throw std::runtime_error(msg);
}

// Parse a redirect (return) directive. Expects at least two args:
// [code, location]. Returns pair<code, location>.
std::pair<http::Status, std::string> Config::parseRedirect(
    const std::vector<std::string>& args) {
  if (args.size() < 2) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Directive requires at least two args";
    throw std::runtime_error(oss.str());
  }
  http::Status code = parseRedirectCode_(args[0]);
  std::string location = args[1];
  return std::make_pair(code, location);
}

http::Status Config::parseStatusCode_(const std::string& value) {
  std::size_t code_sz = parsePositiveNumber_(value);
  int code = static_cast<int>(code_sz);

  if (code_sz > static_cast<std::size_t>(INT_MAX) ||
      !http::isValidStatusCode(code)) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "Invalid status code " << code_sz;
    std::string msg = oss.str();
    LOG(ERROR) << msg;
    throw std::runtime_error(msg);
  }

  return http::intToStatus(code);
}

// ==================== TRANSLATION/BUILDING METHODS ====================

void Config::translateServerBlock_(const BlockNode& server_block, Server& srv,
                                   size_t server_index) {
  LOG(DEBUG) << "Translating server block #" << server_index << "...";

  // set current server context for helpers
  current_server_index_ = server_index;
  current_location_path_.clear();

  // Process server directives (handle listen + others in one pass)
  LOG(DEBUG) << "Processing " << server_block.directives.size()
             << " server directive(s)";
  for (size_t i = 0; i < server_block.directives.size(); ++i) {
    const DirectiveNode& d = server_block.directives[i];

    if (d.name == "listen") {
      requireArgsEqual_(d, 1);
      Config::ListenInfo li = parseListen(d.args[0]);
      srv.port = li.port;
      srv.host = li.host;
      LOG(DEBUG) << "Server listen: " << inet_ntoa(*(in_addr*)&srv.host) << ":"
                 << srv.port;

    } else if (d.name == "root") {
      requireArgsEqual_(d, 1);
      srv.root = d.args[0];
      LOG(DEBUG) << "Server root: " << srv.root;

    } else if (d.name == "index") {
      requireArgsEqual_(d, 1);
      std::set<std::string> idx;
      for (size_t j = 0; j < d.args.size(); ++j) {
        idx.insert(trim_copy(d.args[j]));
      }
      srv.index = idx;
      LOG(DEBUG) << "Server index files: " << d.args.size() << " file(s)";

    } else if (d.name == "autoindex") {
      requireArgsAtLeast_(d, 1);
      srv.autoindex = parseBooleanValue_(d.args[0]);
      LOG(DEBUG) << "Server autoindex: " << (srv.autoindex ? "on" : "off");

    } else if (d.name == "allow_methods") {
      requireArgsAtLeast_(d, 1);
      srv.allow_methods = parseMethods(d.args);
      LOG(DEBUG) << "Server allowed methods: " << d.args.size() << " method(s)";

    } else if (d.name == "error_page") {
      requireArgsAtLeast_(d, 2);
      std::map<http::Status, std::string> parsed = parseErrorPages(d.args);
      for (std::map<http::Status, std::string>::const_iterator it =
               parsed.begin();
           it != parsed.end(); ++it) {
        srv.error_page[it->first] = it->second;
        LOG(DEBUG) << "Server error_page: " << it->first << " -> "
                   << it->second;
      }

    } else if (d.name == "max_request_body") {
      requireArgsEqual_(d, 1);
      srv.max_request_body = parsePositiveNumber_(d.args[0]);
      LOG(DEBUG) << "Server max_request_body: " << srv.max_request_body;
    } else {
      throwUnrecognizedDirective_(d, "in server block");
    }
  }

  // NOTE: restoration of context will happen at function end so subsequent
  // servers are processed with the global context.

  // Apply global error pages if not overridden
  if (srv.error_page.empty()) {
    srv.error_page = global_error_pages_;
    LOG(DEBUG) << "Applied global error pages to server";
  }

  // Minimum requirements: ensure listen was specified and root is set
  if (srv.port <= 0) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "server #" << server_index
        << " missing 'listen' directive or invalid port";
    std::string msg = oss.str();
    LOG(ERROR) << msg;
    throw std::runtime_error(msg);
  }
  if (srv.root.empty()) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "server #" << server_index
        << " missing 'root' directive";
    std::string msg = oss.str();
    LOG(ERROR) << msg;
    throw std::runtime_error(msg);
  }

  // max_request_body inheritance: global -> server -> default
  // If server didn't set it and global did, use global
  // If neither set it, use DEFAULT
  if (srv.max_request_body == kMaxRequestBodyUnset) {
    if (global_max_request_body_ != kMaxRequestBodyUnset) {
      srv.max_request_body = global_max_request_body_;
      LOG(DEBUG) << "Applied global max_request_body to server: "
                 << srv.max_request_body;
    } else {
      srv.max_request_body = kMaxRequestBodyDefault;
      LOG(DEBUG) << "Applied default max_request_body to server: "
                 << srv.max_request_body;
    }
  }

  LOG(DEBUG) << "Processing " << server_block.sub_blocks.size()
             << " location block(s)";
  for (size_t i = 0; i < server_block.sub_blocks.size(); ++i) {
    const BlockNode& block = server_block.sub_blocks[i];
    if (block.type == "location") {
      LOG(DEBUG) << "Translating location: " << block.param;
      Location loc(block.param);
      translateLocationBlock_(block, loc);
      srv.locations[loc.path] = loc;
    }
  }
  LOG(DEBUG) << "Server block translation completed";
  // restore to global context
  current_server_index_ = kGlobalContext;
  current_location_path_.clear();
}

void Config::translateLocationBlock_(const BlockNode& location_block,
                                     Location& loc) {
  // Set path from constructor parameter (block.param)
  loc.path = location_block.param;
  LOG(DEBUG) << "Translating location block: " << loc.path;
  // set current location context (server_index already set by caller)
  current_location_path_ = loc.path;

  // Parse directives
  LOG(DEBUG) << "Processing " << location_block.directives.size()
             << " location directive(s)";
  for (size_t i = 0; i < location_block.directives.size(); ++i) {
    const DirectiveNode& d = location_block.directives[i];

    if (d.name == "root") {
      requireArgsEqual_(d, 1);
      loc.root = d.args[0];
      LOG(DEBUG) << "  Location root: " << loc.root;
    } else if (d.name == "index") {
      requireArgsAtLeast_(d, 1);
      std::set<std::string> idx;
      for (size_t j = 0; j < d.args.size(); ++j) {
        idx.insert(trim_copy(d.args[j]));
      }
      loc.index = idx;
      LOG(DEBUG) << "  Location index files: " << d.args.size() << " file(s)";
    } else if (d.name == "autoindex") {
      requireArgsEqual_(d, 1);
      loc.autoindex = parseBooleanValue_(d.args[0]) ? ON : OFF;
      LOG(DEBUG) << "  Location autoindex: "
                 << (loc.autoindex == ON ? "on" : "off");
    } else if (d.name == "allow_methods") {
      requireArgsAtLeast_(d, 1);
      loc.allow_methods = parseMethods(d.args);
      LOG(DEBUG) << "  Location allowed methods: " << d.args.size()
                 << " method(s)";
    } else if (d.name == "redirect") {
      requireArgsEqual_(d, 2);
      std::pair<http::Status, std::string> ret = parseRedirect(d.args);
      loc.redirect_code = ret.first;
      loc.redirect_location = ret.second;
      LOG(DEBUG) << "  Location redirect: " << loc.redirect_code << " -> "
                 << loc.redirect_location;
    } else if (d.name == "error_page") {
      requireArgsAtLeast_(d, 2);
      std::map<http::Status, std::string> parsed = parseErrorPages(d.args);
      for (std::map<http::Status, std::string>::const_iterator it =
               parsed.begin();
           it != parsed.end(); ++it) {
        loc.error_page[it->first] = it->second;
        LOG(DEBUG) << "  Location error_page: " << it->first << " -> "
                   << it->second;
      }
    } else if (d.name == "cgi_root") {
      requireArgsEqual_(d, 1);
      loc.cgi_root = d.args[0];
      LOG(DEBUG) << "  Location CGI root: " << loc.cgi_root;
    } else if (d.name == "cgi_extensions") {
      requireArgsAtLeast_(d, 1);
      std::set<std::string> exts;
      for (size_t j = 0; j < d.args.size(); ++j) {
        std::string ext = trim_copy(d.args[j]);
        // Ensure extension starts with a dot
        if (!ext.empty() && ext[0] != '.') {
          ext = "." + ext;
        }
        exts.insert(ext);
      }
      loc.cgi_extensions = exts;
      LOG(DEBUG) << "  Location CGI extensions: " << d.args.size()
                 << " extension(s)";
    } else if (d.name == "max_request_body") {
      requireArgsEqual_(d, 1);
      loc.max_request_body = parsePositiveNumber_(d.args[0]);
      LOG(DEBUG) << "  Location max_request_body: " << loc.max_request_body;
    } else {
      throwUnrecognizedDirective_(d, "in location block");
    }
  }

  // Validate: location cannot have both CGI and redirect
  if (!loc.cgi_root.empty() && loc.redirect_code != http::S_0_UNKNOWN) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "location '" << loc.path
        << "' cannot have both 'cgi_root' and 'redirect' directives";
    std::string msg = oss.str();
    LOG(ERROR) << msg;
    throw std::runtime_error(msg);
  }

  // Validate: if cgi is enabled, cgi_extensions must be configured
  if (!loc.cgi_root.empty() && loc.cgi_extensions.empty()) {
    std::ostringstream oss;
    oss << configErrorPrefix() << "location '" << loc.path
        << "' has 'cgi_root' set but 'cgi_extensions' is not configured";
    std::string msg = oss.str();
    LOG(ERROR) << msg;
    throw std::runtime_error(msg);
  }

  // clear location context (server context remains active in caller)
  current_location_path_.clear();
  LOG(DEBUG) << "Location block translation completed: " << loc.path;
}

// ==================== DIRECTIVE PARSERS ====================

Config::ListenInfo Config::parseListen(const std::string& listen_arg) {
  Config::ListenInfo li;
  size_t colon_pos = listen_arg.find(':');

  // Extract port string
  std::string portstr;
  if (colon_pos != std::string::npos) {
    portstr = listen_arg.substr(colon_pos + 1);
  } else {
    portstr = listen_arg;
  }
  li.port = parsePortValue_(portstr);

  // no host
  if (colon_pos == std::string::npos) {
    li.host = INADDR_ANY;
    return li;
  }

  li.host = inet_addr(listen_arg.substr(0, colon_pos).c_str());

  // invalid host
  if (li.host == INADDR_NONE) {
    std::ostringstream oss;
    oss << configErrorPrefix()
        << "Invalid IP address in listen directive: " << listen_arg;
    LOG(ERROR) << oss.str();
    throw std::runtime_error(oss.str());
  }

  return li;
}
