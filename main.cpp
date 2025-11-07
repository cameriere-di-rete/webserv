#include "Config.hpp"
#include "ConfigTranslator.hpp"
#include "ConfigValidator.hpp"
#include "ServerManager.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  const char *path = (argc > 1) ? argv[1] : "./webserv.conf";

  std::vector<Server> servers;
  std::map<int, std::string> global_error_pages;
  std::size_t global_max_request_body;

  // ===== CONFIGURATION PHASE =====
  // 1. Parse config file into BlockNode tree
  // 2. Translate BlockNode tree to Server objects with directive maps
  // 3. Validate all directives and settings
  try {
    Config cfg;
    cfg.parseFile(std::string(path));
    BlockNode root = cfg.getRoot();

    // Translate parsed config tree into Server objects
    // Populates Server.directives map and Server.locations map
    servers = ConfigTranslator::translateConfig(root, global_error_pages,
                                                 global_max_request_body);

    // Validate all server configurations (ports, paths, methods, etc.)
    // Throws exception on invalid configuration
    ConfigValidator::validateServers(servers);

    if (servers.empty()) {
      return error("Error: no valid 'server' blocks found in configuration");
    }

  } catch (const std::exception &e) {
    return error(std::string("Error: could not read/parse config: ") +
                 e.what());
  }

  // ===== SERVER INITIALIZATION =====
  // Initialize sockets and prepare for accepting connections
  ServerManager sm;
  try {
    sm.initServers(servers);
  } catch (const std::exception &e) {
    return error(e.what());
  } catch (...) {
    return error("Unknown error while initializing Server");
  }

  // ===== EVENT LOOP =====
  // Ignore SIGPIPE to prevent crashes when clients disconnect
  signal(SIGPIPE, SIG_IGN);

  // Enter main event loop (epoll-based I/O multiplexing)
  return sm.run();
}
