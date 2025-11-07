#pragma once

#include "BlockNode.hpp"
#include "Location.hpp"
#include "Server.hpp"
#include <map>
#include <string>
#include <vector>

class ConfigTranslator {
public:
  // Translates parsed config tree into Server objects with directive maps
  // Extracts global directives and returns vector of configured servers
  static std::vector<Server>
  translateConfig(const BlockNode &root,
                  std::map<int, std::string> &global_error_pages,
                  std::size_t &global_max_request_body);

  // Translates a server block into Server object
  // Populates directives map and locations map, inherits global settings
  static Server translateServerBlock(const BlockNode &server_block,
                                      const std::map<int, std::string> &global_error_pages,
                                      std::size_t global_max_request_body);

  // Translates a location block into Location object
  // Populates directives map with location-specific settings
  static Location translateLocationBlock(const BlockNode &location_block);

private:
  ConfigTranslator();
  ConfigTranslator(const ConfigTranslator &other);
  ConfigTranslator &operator=(const ConfigTranslator &other);
  ~ConfigTranslator();

  // Extracts port number from listen directive (e.g., "0.0.0.0:8080" -> 8080)
  static int parsePort(const std::string &listen_arg);

  // Extracts host from listen directive (e.g., "0.0.0.0:8080" -> "0.0.0.0")
  static std::string parseHost(const std::string &listen_arg);

  // Converts string to boolean (on/off, true/false, 1/0)
  static bool parseBool(const std::string &value);
};
