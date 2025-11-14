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

  void parseFile(const std::string &path);
  std::vector<Server> getServers(void);
  void debug(void) const;

private:
  std::vector<std::string> tokens_;
  BlockNode root_;
  std::vector<Server> servers_;
  std::map<int, std::string> global_error_pages_;
  std::size_t global_max_request_body_;
  size_t idx_;
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

  // Argument parsers
  int parsePortValue_(int port);
  bool parseBooleanValue_(const std::string &value);
  http::Method parseHttpMethod_(const std::string &method);
  int parseRedirectCode_(const std::string &value);
  std::size_t parsePositiveNumberValue_(const std::string &value);
  std::string parsePath_(const std::string &path);
  // Return-style parse helpers (convert+validate and return the value)
  std::set<http::Method> parseMethods(const std::vector<std::string> &args);
  std::map<int, std::string>
  parseErrorPages(const std::vector<std::string> &args);
  std::pair<int, std::string>
  parseRedirect(const std::vector<std::string> &args);
  int parseStatusCode_(const std::string &value);
  struct ListenInfo {
    in_addr_t host;
    int port;
  };
  ListenInfo parseListen(const std::string &listen_arg);

  bool isPositiveNumber_(const std::string &value);

  std::string configError(const std::string &detail) const;
  std::string configErrorPrefix() const;

  // Translation/building methods
  void translateServerBlock_(const BlockNode &server_block, Server &srv,
                             size_t server_index);
  void translateLocationBlock_(const BlockNode &location_block, Location &loc);
};
