#pragma once

#include "Server.hpp"
#include <string>
#include <vector>

class ConfigValidator {
public:
  // Validates all servers and their directives, throws on invalid configuration
  static void validateServers(const std::vector<Server> &servers);

private:
  ConfigValidator();
  ConfigValidator(const ConfigValidator &other);
  ConfigValidator &operator=(const ConfigValidator &other);
  ~ConfigValidator();

  // Validates a single server's directives and all its locations
  static void validateServer(const Server &srv, size_t server_index);

  // Validates a single location's directives
  static void validateLocation(const Location &loc, size_t server_index,
                                const std::string &location_path);

  // Validates port is in range 1-65535
  static void validatePort(int port, size_t server_index);

  // Validates boolean value is one of: on/off, true/false, 1/0
  static void validateBooleanValue(const std::string &value,
                                    const std::string &directive,
                                    size_t server_index,
                                    const std::string &location_path);

  // Validates HTTP method is one of: GET, POST, DELETE, HEAD, PUT
  static void validateHttpMethod(const std::string &method,
                                  const std::string &directive,
                                  size_t server_index,
                                  const std::string &location_path);

  // Validates redirect code is one of: 301, 302, 303, 307, 308
  static void validateRedirectCode(int code, size_t server_index,
                                    const std::string &location_path);

  // Validates value is a positive number
  static void validatePositiveNumber(const std::string &value,
                                      const std::string &directive,
                                      size_t server_index,
                                      const std::string &location_path);

  // Validates path exists and is a directory
  static void validatePath(const std::string &path, const std::string &directive,
                            size_t server_index,
                            const std::string &location_path);

  // Checks if method is a valid HTTP method
  static bool isValidHttpMethod(const std::string &method);

  // Checks if code is a valid redirect status code
  static bool isValidRedirectCode(int code);

  // Checks if value is a positive number
  static bool isPositiveNumber(const std::string &value);
};
