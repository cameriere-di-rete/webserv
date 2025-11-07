#include "ConfigValidator.hpp"
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>

ConfigValidator::ConfigValidator() {}
ConfigValidator::ConfigValidator(const ConfigValidator &other) { (void)other; }
ConfigValidator &ConfigValidator::operator=(const ConfigValidator &other) {
  (void)other;
  return *this;
}
ConfigValidator::~ConfigValidator() {}

void ConfigValidator::validateServers(const std::vector<Server> &servers) {
  if (servers.empty()) {
    throw std::runtime_error("Configuration error: No server blocks defined");
  }

  for (size_t i = 0; i < servers.size(); ++i) {
    validateServer(servers[i], i);
  }
}

void ConfigValidator::validateServer(const Server &srv, size_t server_index) {
  // Validate port
  validatePort(srv.port, server_index);

  // Validate server directives
  for (std::map<std::string, std::vector<std::string> >::const_iterator it =
           srv.directives.begin();
       it != srv.directives.end(); ++it) {
    const std::string &directive = it->first;
    const std::vector<std::string> &args = it->second;

    if (args.empty()) {
      std::ostringstream oss;
      oss << "Configuration error in server #" << server_index
          << ": Directive '" << directive << "' has no arguments";
      throw std::runtime_error(oss.str());
    }

    // Validate specific directives
    if (directive == "autoindex") {
      validateBooleanValue(args[0], directive, server_index, "");
    } else if (directive == "root") {
      validatePath(args[0], directive, server_index, "");
    } else if (directive == "max_request_body") {
      validatePositiveNumber(args[0], directive, server_index, "");
    } else if (directive == "method") {
      for (size_t j = 0; j < args.size(); ++j) {
        validateHttpMethod(args[j], directive, server_index, "");
      }
    }
  }

  // Validate locations
  for (std::map<std::string, Location>::const_iterator loc_it =
           srv.locations.begin();
       loc_it != srv.locations.end(); ++loc_it) {
    validateLocation(loc_it->second, server_index, loc_it->first);
  }
}

void ConfigValidator::validateLocation(const Location &loc,
                                        size_t server_index,
                                        const std::string &location_path) {
  // Validate location directives
  for (std::map<std::string, std::vector<std::string> >::const_iterator it =
           loc.directives.begin();
       it != loc.directives.end(); ++it) {
    const std::string &directive = it->first;
    const std::vector<std::string> &args = it->second;

    if (args.empty()) {
      std::ostringstream oss;
      oss << "Configuration error in server #" << server_index << " location '"
          << location_path << "': Directive '" << directive
          << "' has no arguments";
      throw std::runtime_error(oss.str());
    }

    // Validate specific directives
    if (directive == "autoindex") {
      validateBooleanValue(args[0], directive, server_index, location_path);
    } else if (directive == "root") {
      validatePath(args[0], directive, server_index, location_path);
    } else if (directive == "method") {
      for (size_t j = 0; j < args.size(); ++j) {
        validateHttpMethod(args[j], directive, server_index, location_path);
      }
    } else if (directive == "return") {
      if (args.size() >= 1) {
        int code = std::atoi(args[0].c_str());
        validateRedirectCode(code, server_index, location_path);
      }
    } else if (directive == "max_request_body") {
      validatePositiveNumber(args[0], directive, server_index, location_path);
    }
  }
}

void ConfigValidator::validatePort(int port, size_t server_index) {
  if (port < 1 || port > 65535) {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index
        << ": Invalid port number " << port << " (must be 1-65535)";
    throw std::runtime_error(oss.str());
  }
}

void ConfigValidator::validateBooleanValue(const std::string &value,
                                            const std::string &directive,
                                            size_t server_index,
                                            const std::string &location_path) {
  if (value != "on" && value != "off" && value != "true" && value != "false" &&
      value != "1" && value != "0") {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index;
    if (!location_path.empty()) {
      oss << " location '" << location_path << "'";
    }
    oss << ": Invalid boolean value '" << value << "' for directive '"
        << directive << "' (expected: on/off, true/false, or 1/0)";
    throw std::runtime_error(oss.str());
  }
}

void ConfigValidator::validateHttpMethod(const std::string &method,
                                          const std::string &directive,
                                          size_t server_index,
                                          const std::string &location_path) {
  if (!isValidHttpMethod(method)) {
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

void ConfigValidator::validateRedirectCode(int code, size_t server_index,
                                            const std::string &location_path) {
  if (!isValidRedirectCode(code)) {
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

void ConfigValidator::validatePositiveNumber(const std::string &value,
                                              const std::string &directive,
                                              size_t server_index,
                                              const std::string &location_path) {
  if (!isPositiveNumber(value)) {
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

void ConfigValidator::validatePath(const std::string &path,
                                    const std::string &directive,
                                    size_t server_index,
                                    const std::string &location_path) {
  struct stat buffer;
  if (stat(path.c_str(), &buffer) != 0) {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index;
    if (!location_path.empty()) {
      oss << " location '" << location_path << "'";
    }
    oss << ": Path '" << path << "' in directive '" << directive
        << "' does not exist";
    throw std::runtime_error(oss.str());
  }

  if (!S_ISDIR(buffer.st_mode)) {
    std::ostringstream oss;
    oss << "Configuration error in server #" << server_index;
    if (!location_path.empty()) {
      oss << " location '" << location_path << "'";
    }
    oss << ": Path '" << path << "' in directive '" << directive
        << "' is not a directory";
    throw std::runtime_error(oss.str());
  }
}

bool ConfigValidator::isValidHttpMethod(const std::string &method) {
  return (method == "GET" || method == "POST" || method == "DELETE" ||
          method == "HEAD" || method == "PUT");
}

bool ConfigValidator::isValidRedirectCode(int code) {
  return (code == 301 || code == 302 || code == 303 || code == 307 ||
          code == 308);
}

bool ConfigValidator::isPositiveNumber(const std::string &value) {
  if (value.empty())
    return false;

  for (size_t i = 0; i < value.size(); ++i) {
    if (value[i] < '0' || value[i] > '9')
      return false;
  }

  long num = std::atol(value.c_str());
  return num > 0;
}
