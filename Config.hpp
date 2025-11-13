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

  // Parsing helpers
  void removeComments(std::string &s);
  void tokenize(const std::string &content);
  bool eof() const;
  const std::string &peek() const;
  std::string get();
  BlockNode parseBlock();
  DirectiveNode parseDirective();

  // Validation methods
  void validateRoot_(void);
  void validatePort_(int port, size_t server_index);
  void validateBooleanValue_(const std::string &value,
                             const std::string &directive, size_t server_index,
                             const std::string &location_path);
  void validateHttpMethod_(const std::string &method,
                           const std::string &directive, size_t server_index,
                           const std::string &location_path);
  void validateRedirectCode_(int code, size_t server_index,
                             const std::string &location_path);
  void validatePositiveNumber_(const std::string &value,
                               const std::string &directive,
                               size_t server_index,
                               const std::string &location_path);
  void validatePath_(const std::string &path, const std::string &directive,
                     size_t server_index, const std::string &location_path);
  // Validate a boolean directive value and populate 'dest' in one call.
  // Keeps validation and population together to avoid duplication.
  void validateAndPopulateBool_(bool &dest, const std::string &value,
                                const std::string &directive,
                                size_t server_index,
                                const std::string &location_path);
  // Validate a list of HTTP methods and populate the destination set
  // (Location::Method enum). Keeps validation and population together.
  void validateAndPopulateMethods_(std::set<Location::Method> &dest,
                                   const std::vector<std::string> &args,
                                   const std::string &directive,
                                   size_t server_index,
                                   const std::string &location_path);
  // Validate a positive numeric directive and populate the destination
  // (e.g., max_request_body). Keeps validation and parsing in one place.
  void validateAndPopulatePositiveNumber_(std::size_t &dest,
                                          const std::string &value,
                                          const std::string &directive,
                                          size_t server_index,
                                          const std::string &location_path);
  // Validate error_page arguments and populate the destination map.
  // Expects last arg to be the path and previous args to be status codes.
  void validateAndPopulateErrorPages_(std::map<int, std::string> &dest,
                                      const std::vector<std::string> &args,
                                      const std::string &directive,
                                      size_t server_index,
                                      const std::string &location_path);
  // Validate a redirect (return) directive and populate code + location
  void validateAndPopulateRedirect_(int &dest_code, std::string &dest_location,
                                    const std::vector<std::string> &args,
                                    const std::string &directive,
                                    size_t server_index,
                                    const std::string &location_path);
  bool isValidHttpMethod_(const std::string &method);
  bool isValidRedirectCode_(int code);
  void validateStatusCode_(int code, size_t server_index,
                           const std::string &location_path);
  bool isValidStatusCode_(int code);
  bool isPositiveNumber_(const std::string &value);

  // Translation/building methods
  void buildServersFromRoot_(void);
  void translateServerBlock_(const BlockNode &server_block, Server &srv,
                             size_t server_index);
  void translateLocationBlock_(const BlockNode &location_block, Location &loc,
                               size_t server_index);
  int parsePort_(const std::string &listen_arg);
  bool parseBool_(const std::string &value);
  void populateBool_(bool &dest, const std::string &value);
};

void dumpConfig(const BlockNode &b);
