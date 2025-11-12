#include "Config.hpp"
#include "Logger.hpp"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

// ==================== PUBLIC METHODS ====================

Config::Config()
    : tokens_(), root_(), servers_(), global_error_pages_(),
      global_max_request_body_(0), idx_(0) {}

Config::~Config() {}

Config::Config(const Config &other)
    : tokens_(other.tokens_), root_(other.root_), servers_(other.servers_),
      global_error_pages_(other.global_error_pages_),
      global_max_request_body_(other.global_max_request_body_),
      idx_(other.idx_) {}

Config &Config::operator=(const Config &other) {
  if (this != &other) {
    tokens_ = other.tokens_;
    idx_ = other.idx_;
    root_ = other.root_;
    servers_ = other.servers_;
    global_error_pages_ = other.global_error_pages_;
    global_max_request_body_ = other.global_max_request_body_;
  }
  return *this;
}

void Config::parseFile(const std::string &path) {
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
    if (peek() == "server") {
      LOG(DEBUG) << "Found server block, parsing...";
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
  // Always perform root validation before building servers.
  LOG(INFO) << "Validating configuration before building servers";
  validateRoot_();
  LOG(INFO) << "Configuration validated successfully";

  if (servers_.empty()) {
    LOG(INFO) << "Building server objects from configuration...";
    buildServersFromRoot_();
    LOG(INFO) << "Built " << servers_.size() << " server(s)";
  }

  return servers_;
}

BlockNode Config::getRoot(void) const {
  return root_;
}

void Config::debug(void) const {
  dumpConfig(root_);
}

// ==================== PARSING HELPERS ====================

void Config::removeComments(std::string &s) {
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

void Config::tokenize(const std::string &content) {
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

const std::string &Config::peek() const {
  static std::string empty = "";
  return idx_ < tokens_.size() ? tokens_[idx_] : empty;
}

std::string Config::get() {
  if (idx_ >= tokens_.size()) {
    throw std::runtime_error("Unexpected end of tokens");
  }
  return tokens_[idx_++];
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
  get(); // consume ;
  return d;
}

BlockNode Config::parseBlock() {
  BlockNode b;
  b.type = get(); // server or location
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
    if (peek() == "location") {
      b.sub_blocks.push_back(parseBlock());
    } else if (peek() == "server") {
      b.sub_blocks.push_back(parseBlock());
    } else {
      b.directives.push_back(parseDirective());
    }
  }
  get(); // consume }
  return b;
}

// ==================== VALIDATION METHODS ====================

void Config::validateRoot_(void) {
  LOG(DEBUG) << "Validating root configuration...";

  // DONE check that sub blocks are `server`
  if (root_.sub_blocks.empty()) {
    LOG(ERROR) << "No server blocks defined in configuration";
    throw std::runtime_error("Configuration error: No server blocks defined");
  }
  // Ensure that all top-level sub-blocks are `server` blocks
  for (size_t i = 0; i < root_.sub_blocks.size(); ++i) {
    const BlockNode &b = root_.sub_blocks[i];
    if (b.type != "server") {
      std::ostringstream oss;
      oss << "Configuration error: unexpected top-level block '" << b.type
          << "' at index " << i << " (expected 'server')";
      LOG(ERROR) << oss.str();
      throw std::runtime_error(oss.str());
    }
  }

  // Parse and validate global directives
  global_max_request_body_ = 0;
  global_error_pages_.clear();

  LOG(DEBUG) << "Processing " << root_.directives.size()
             << " global directive(s)";
  for (size_t i = 0; i < root_.directives.size(); ++i) {
    const DirectiveNode &d = root_.directives[i];

    // DONE throw error when there are no arguments
    if (d.name == "error_page") {
      if (d.args.size() >= 2) {
        std::string path = d.args[d.args.size() - 1];
        for (size_t j = 0; j < d.args.size() - 1; ++j) {
          // DONE validate error code
          int code = std::atoi(d.args[j].c_str());
          if (code > 0) {
            validateStatusCode_(code, 0, "");
            global_error_pages_[code] = path;
            LOG(DEBUG) << "Global error_page: " << code << " -> " << path;
          }
        }
      } else {
        LOG(ERROR) << "Directive 'error_page' requires at least two arguments";
        throw std::runtime_error("Configuration error: 'error_page' directive "
                                 "requires at least two arguments");
      }
    } else if (d.name == "max_request_body" && d.args.size() >= 1) {
      // DONE parse number only once
      const std::string &val_str = d.args[0];
      if (val_str.empty()) {
        LOG(ERROR) << "Invalid max_request_body value: " << val_str;
        throw std::runtime_error(
            "Configuration error: Invalid max_request_body value");
      }
      // ensure string contains only digits
      for (size_t k = 0; k < val_str.size(); ++k) {
        if (val_str[k] < '0' || val_str[k] > '9') {
          LOG(ERROR) << "Invalid max_request_body value: " << val_str;
          throw std::runtime_error(
              "Configuration error: Invalid max_request_body value");
        }
      }
      long parsed = std::atol(val_str.c_str());
      if (parsed <= 0) {
        LOG(ERROR) << "Invalid max_request_body value: " << val_str;
        throw std::runtime_error(
            "Configuration error: Invalid max_request_body value");
      }
      global_max_request_body_ = static_cast<std::size_t>(parsed);
      LOG(DEBUG) << "Global max_request_body set to: "
                 << global_max_request_body_;
    }
  }

  LOG(DEBUG) << "Root validation completed";
}

void Config::validatePort_(int port, size_t server_index) {
  if (port < 1 || port > 65535) {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index
        << ": Invalid port number " << port << " (must be 1-65535)";
    throw std::runtime_error(oss.str());
  }
}

void Config::validateBooleanValue_(const std::string &value,
                                   const std::string &directive,
                                   size_t server_index,
                                   const std::string &location_path) {
  // DONE only check `on`/`off`
  if (value != "on" && value != "off") {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index;
    if (!location_path.empty()) {
      oss << " location '" << location_path << "'";
    }
    oss << ": Invalid boolean value '" << value << "' for directive '"
        << directive << "' (expected: on/off)";
    throw std::runtime_error(oss.str());
  }
}

void Config::validateHttpMethod_(const std::string &method,
                                 const std::string &directive,
                                 size_t server_index,
                                 const std::string &location_path) {
  if (!isValidHttpMethod_(method)) {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index;
    if (!location_path.empty()) {
      oss << " location '" << location_path << "'";
    }
    oss << ": Invalid HTTP method '" << method << "' in directive '"
        << directive << "' (valid: GET, POST, DELETE, HEAD, PUT)";
    throw std::runtime_error(oss.str());
  }
}

void Config::validateRedirectCode_(int code, size_t server_index,
                                   const std::string &location_path) {
  if (!isValidRedirectCode_(code)) {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index;
    if (!location_path.empty()) {
      oss << " location '" << location_path << "'";
    }
    oss << ": Invalid redirect status code " << code
        << " (valid: 301, 302, 303, 307, 308)";
    throw std::runtime_error(oss.str());
  }
}

void Config::validatePositiveNumber_(const std::string &value,
                                     const std::string &directive,
                                     size_t server_index,
                                     const std::string &location_path) {
  if (!isPositiveNumber_(value)) {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index;
    if (!location_path.empty()) {
      oss << " location '" << location_path << "'";
    }
    oss << ": Invalid positive number '" << value << "' for directive '"
        << directive << "'";
    throw std::runtime_error(oss.str());
  }
}

// DONE don't check if path exists
void Config::validatePath_(const std::string &path,
                           const std::string &directive, size_t server_index,
                           const std::string &location_path) {
  // Intentionally do not perform filesystem checks here.
  // The configuration should be validated for syntax and semantics only;
  // existence and permissions are environment/runtime concerns.
  if (path.empty()) {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index;
    if (!location_path.empty()) {
      oss << " location '" << location_path << "'";
    }
    oss << ": Path provided to directive '" << directive << "' is empty";
    throw std::runtime_error(oss.str());
  }
  // Accept the path as-is; callers will handle resolution/IO at runtime.
}

bool Config::isValidHttpMethod_(const std::string &method) {
  return (method == "GET" || method == "POST" || method == "DELETE" ||
          method == "HEAD" || method == "PUT");
}

bool Config::isValidRedirectCode_(int code) {
  return (code == 301 || code == 302 || code == 303 || code == 307 ||
          code == 308);
}

bool Config::isValidStatusCode_(int code) {
  return (code >= 100 && code <= 599);
}

void Config::validateStatusCode_(int code, size_t server_index,
                                 const std::string &location_path) {
  if (!isValidStatusCode_(code)) {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index;
    if (!location_path.empty()) {
      oss << " location '" << location_path << "'";
    }
    oss << ": Invalid status code " << code;
    LOG(ERROR) << oss.str();
    throw std::runtime_error(oss.str());
  }
}

bool Config::isPositiveNumber_(const std::string &value) {
  if (value.empty()) {
    return false;
  }

  for (size_t i = 0; i < value.size(); ++i) {
    if (value[i] < '0' || value[i] > '9') {
      return false;
    }
  }

  long num = std::atol(value.c_str());
  return num > 0;
}

// ==================== TRANSLATION/BUILDING METHODS ====================

void Config::buildServersFromRoot_(void) {
  LOG(DEBUG) << "Building servers from root configuration...";
  servers_.clear();

  for (size_t i = 0; i < root_.sub_blocks.size(); ++i) {
    const BlockNode &block = root_.sub_blocks[i];
    if (block.type == "server") {
      LOG(DEBUG) << "Translating server block #" << i;
      Server srv;
      translateServerBlock_(block, srv, i);
      servers_.push_back(srv);
      LOG(INFO) << "Server #" << i << " created - Port: " << srv.port
                << ", Locations: " << srv.locations.size();
    }
  }
  LOG(DEBUG) << "Finished building servers";
}

void Config::translateServerBlock_(const BlockNode &server_block, Server &srv,
                                   size_t server_index) {
  LOG(DEBUG) << "Translating server block #" << server_index << "...";

  // Parse listen directive first
  for (size_t i = 0; i < server_block.directives.size(); ++i) {
    const DirectiveNode &d = server_block.directives[i];
    if (d.name == "listen" && d.args.size() >= 1) {
      srv.port = parsePort_(d.args[0]);
      validatePort_(srv.port, server_index);

      std::string host_str = parseHost_(d.args[0]);
      // For now, use INADDR_ANY (0). In future, can parse host_str
      srv.host = 0;
      LOG(DEBUG) << "Server listen: " << host_str << ":" << srv.port;
      break;
    }
    // TODO to check minimum requirements, fill and validate root as well
  }

  // Parse other directives
  LOG(DEBUG) << "Processing " << server_block.directives.size()
             << " server directive(s)";
  for (size_t i = 0; i < server_block.directives.size(); ++i) {
    const DirectiveNode &d = server_block.directives[i];

    if (d.name == "listen") {
      // Already handled
      continue;
    } else if (d.name == "root" && !d.args.empty()) {
      validatePath_(d.args[0], d.name, 0, "");
      srv.root = d.args[0];
      LOG(DEBUG) << "Server root: " << srv.root;
    } else if (d.name == "index" && !d.args.empty()) {
      srv.index.clear();
      for (size_t j = 0; j < d.args.size(); ++j) {
        srv.index.insert(d.args[j]);
      }
      LOG(DEBUG) << "Server index files: " << d.args.size() << " file(s)";
    } else if (d.name == "autoindex" && !d.args.empty()) {
      // TODO validate and populate at the same time
      validateBooleanValue_(d.args[0], d.name, 0, "");
      populateBool_(srv.autoindex, d.args[0]);
      LOG(DEBUG) << "Server autoindex: " << (srv.autoindex ? "on" : "off");
    } else if (d.name == "allow_methods" && !d.args.empty()) {
      srv.allow_methods.clear();
      for (size_t j = 0; j < d.args.size(); ++j) {
        // TODO validate and populate at the same time
        validateHttpMethod_(d.args[j], d.name, 0, "");
        if (d.args[j] == "GET") {
          srv.allow_methods.insert(Location::GET);
        } else if (d.args[j] == "POST") {
          srv.allow_methods.insert(Location::POST);
        } else if (d.args[j] == "PUT") {
          srv.allow_methods.insert(Location::PUT);
        } else if (d.args[j] == "DELETE") {
          srv.allow_methods.insert(Location::DELETE);
        } else if (d.args[j] == "HEAD") {
          srv.allow_methods.insert(Location::HEAD);
        }
      }
      LOG(DEBUG) << "Server allowed methods: " << d.args.size() << " method(s)";
    } else if (d.name == "error_page" && d.args.size() >= 2) {
      std::string path = d.args[d.args.size() - 1];
      for (size_t j = 0; j < d.args.size() - 1; ++j) {
        // TODO validate and populate at the same time
        int code = std::atoi(d.args[j].c_str());
        if (code > 0) {
          validateStatusCode_(code, server_index, "");
          srv.error_page[code] = path;
          LOG(DEBUG) << "Server error_page: " << code << " -> " << path;
        }
      }
    } else if (d.name == "max_request_body" && !d.args.empty()) {
      // TODO validate and populate at the same time
      validatePositiveNumber_(d.args[0], d.name, 0, "");
      srv.max_request_body =
          static_cast<std::size_t>(std::atol(d.args[0].c_str()));
      LOG(DEBUG) << "Server max_request_body: " << srv.max_request_body;
    }
  }

  // Apply global error pages if not overridden
  if (srv.error_page.empty()) {
    srv.error_page = global_error_pages_;
    LOG(DEBUG) << "Applied global error pages to server";
  }

  // Apply global max_request_body if not set
  if (srv.max_request_body == 0 && global_max_request_body_ > 0) {
    srv.max_request_body = global_max_request_body_;
    LOG(DEBUG) << "Applied global max_request_body to server: "
               << srv.max_request_body;
  }

  // Parse location blocks
  LOG(DEBUG) << "Processing " << server_block.sub_blocks.size()
             << " location block(s)";
  for (size_t i = 0; i < server_block.sub_blocks.size(); ++i) {
    const BlockNode &block = server_block.sub_blocks[i];
    if (block.type == "location") {
      LOG(DEBUG) << "Translating location: " << block.param;
      Location loc(block.param);
      translateLocationBlock_(block, loc, server_index);
      srv.locations[loc.path] = loc;
    }
  }
  LOG(DEBUG) << "Server block translation completed";
}

void Config::translateLocationBlock_(const BlockNode &location_block,
                                     Location &loc, size_t server_index) {
  // Set path from constructor parameter (block.param)
  loc.path = location_block.param;
  LOG(DEBUG) << "Translating location block: " << loc.path;
  (void)server_index;

  // Parse directives
  LOG(DEBUG) << "Processing " << location_block.directives.size()
             << " location directive(s)";
  for (size_t i = 0; i < location_block.directives.size(); ++i) {
    const DirectiveNode &d = location_block.directives[i];

    if (d.name == "root" && !d.args.empty()) {
      validatePath_(d.args[0], d.name, 0, loc.path);
      loc.root = d.args[0];
      LOG(DEBUG) << "  Location root: " << loc.root;
    } else if (d.name == "index" && !d.args.empty()) {
      loc.index.clear();
      for (size_t j = 0; j < d.args.size(); ++j) {
        loc.index.insert(d.args[j]);
      }
      LOG(DEBUG) << "  Location index files: " << d.args.size() << " file(s)";
    } else if (d.name == "autoindex" && !d.args.empty()) {
      // TODO validate and populate at the same time
      validateBooleanValue_(d.args[0], d.name, 0, loc.path);
      populateBool_(loc.autoindex, d.args[0]);
      LOG(DEBUG) << "  Location autoindex: " << (loc.autoindex ? "on" : "off");
    } else if (d.name == "allow_methods" && !d.args.empty()) {
      loc.allow_methods.clear();
      for (size_t j = 0; j < d.args.size(); ++j) {
        validateHttpMethod_(d.args[j], d.name, 0, loc.path);
        if (d.args[j] == "GET") {
          loc.allow_methods.insert(Location::GET);
        } else if (d.args[j] == "POST") {
          loc.allow_methods.insert(Location::POST);
        } else if (d.args[j] == "PUT") {
          loc.allow_methods.insert(Location::PUT);
        } else if (d.args[j] == "DELETE") {
          loc.allow_methods.insert(Location::DELETE);
        } else if (d.args[j] == "HEAD") {
          loc.allow_methods.insert(Location::HEAD);
        }
      }
      LOG(DEBUG) << "  Location allowed methods: " << d.args.size()
                 << " method(s)";
    } else if (d.name == "return" && d.args.size() >= 2) {
      // TODO validate and populate at the same time
      int code = std::atoi(d.args[0].c_str());
      validateRedirectCode_(code, 0, loc.path);
      loc.redirect_code = code;
      loc.redirect_location = d.args[1];
      LOG(DEBUG) << "  Location redirect: " << code << " -> "
                 << loc.redirect_location;
    } else if (d.name == "error_page" && d.args.size() >= 2) {
      std::string path = d.args[d.args.size() - 1];
      for (size_t j = 0; j < d.args.size() - 1; ++j) {
        int code = std::atoi(d.args[j].c_str());
        if (code > 0) {
          // TODO validate error code, not redirect
          validateRedirectCode_(code, 0, loc.path);
          loc.error_page[code] = path;
          LOG(DEBUG) << "  Location error_page: " << code << " -> " << path;
        }
      }
    } else if (d.name == "cgi" && !d.args.empty()) {
      validateBooleanValue_(d.args[0], d.name, 0, loc.path);
      populateBool_(loc.cgi, d.args[0]);
      LOG(DEBUG) << "  Location CGI: " << (loc.cgi ? "on" : "off");
    }
  }
  LOG(DEBUG) << "Location block translation completed: " << loc.path;
}

int Config::parsePort_(const std::string &listen_arg) {
  std::string portstr;
  size_t colon_pos = listen_arg.find(':');
  if (colon_pos != std::string::npos) {
    portstr = listen_arg.substr(colon_pos + 1);
  } else {
    portstr = listen_arg;
  }
  return std::atoi(portstr.c_str());
}

// TODO use address type instead of string
std::string Config::parseHost_(const std::string &listen_arg) {
  size_t colon_pos = listen_arg.find(':');
  if (colon_pos != std::string::npos) {
    return listen_arg.substr(0, colon_pos);
  }
  return "0.0.0.0";
}

bool Config::parseBool_(const std::string &value) {
  return (value == "on" || value == "true" || value == "1");
}

void Config::populateBool_(bool &dest, const std::string &value) {
  dest = parseBool_(value);
}

// ==================== DEBUG OUTPUT ====================

static void _printBlockRec(const BlockNode &b, int indent) {
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
    const DirectiveNode &d = b.directives[i];
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

void dumpConfig(const BlockNode &b) {
  _printBlockRec(b, 0);
}
