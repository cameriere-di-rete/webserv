#pragma once

#include "BlockNode.hpp"
#include "DirectiveNode.hpp"
#include "Server.hpp"
#include <string>
#include <vector>

class Config {
public:
  Config();
  ~Config();
  Config(const Config &other);
  Config &operator=(const Config &other);

  // Phase 1: Parse the config file into BlockNode tree
  // Throws std::runtime_error on syntax errors.
  void parseFile(const std::string &path);

  // Phase 2+3: Validate the parsed configuration (if needed) and build
  // Server objects. This method will perform validation automatically if
  // it hasn't been run yet.
  // Throws std::runtime_error on validation errors.
  std::vector<Server> getServers(void);

  // Debug output
  BlockNode getRoot(void) const;
  void debug(void) const;

private:
  std::vector<std::string> tokens_;
  BlockNode root_;
  std::vector<Server> servers_;
  std::map<int, std::string> global_error_pages_;
  std::size_t global_max_request_body_;
  size_t idx_;
  // Current validation context (used to avoid passing server_index +
  // location_path to every helper). kGlobalContext indicates top-level
  // (no server) context.
  static const size_t kGlobalContext = static_cast<size_t>(-1);
  size_t current_server_index_;
  std::string current_location_path_;

  // Parsing helpers
  void removeComments(std::string &s);
  void tokenize(const std::string &content);
  bool eof() const;
  const std::string &peek() const;
  std::string get();
  BlockNode parseBlock();
  DirectiveNode parseDirective();

  // Validation methods
  // Convert+validate helpers: convert an input string (or primitive) to
  // a typed value, validating along the way and returning it. They throw
  // std::runtime_error on invalid values.
  int parsePortValue_(int port);
  bool parseBooleanValue_(const std::string &value);
  Location::Method parseHttpMethod_(const std::string &method);
  int parseRedirectCode_(const std::string &value);
  std::size_t parsePositiveNumberValue_(const std::string &value);
  std::string parsePath_(const std::string &path);
  // Return-style parse helpers (convert+validate and return the value)
  std::set<Location::Method> parseMethods(const std::vector<std::string> &args);
  std::map<int, std::string>
  parseErrorPages(const std::vector<std::string> &args);
  std::pair<int, std::string>
  parseRedirect(const std::vector<std::string> &args);
  bool isValidHttpMethod_(const std::string &method);
  bool isValidRedirectCode_(int code);
  int parseStatusCode_(const std::string &value);
  bool isValidStatusCode_(int code);
  bool isPositiveNumber_(const std::string &value);

  // Translation/building methods
  void translateServerBlock_(const BlockNode &server_block, Server &srv,
                             size_t server_index);
  void translateLocationBlock_(const BlockNode &location_block, Location &loc);
  int parsePort_(const std::string &listen_arg);
  // Directive-specific parsers (validate and convert to typed values)
  struct ListenInfo {
    in_addr_t host;
    int port;
  };
  ListenInfo parseListen(const std::string &listen_arg);
};
