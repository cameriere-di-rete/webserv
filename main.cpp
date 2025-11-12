#include "Config.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  LOG(INFO) << "=================================================";
  LOG(INFO) << "          WebServer Starting Up";
  LOG(INFO) << "=================================================";

  const char *path = (argc > 1) ? argv[1] : "./webserv.conf";
  LOG(INFO) << "Using configuration file: " << path;

  std::vector<Server> servers;

  // ===== CONFIGURATION PHASE =====
  // 1. Parse config file into BlockNode tree
  // 2. Validate the parsed configuration
  // 3. Build Server objects from validated config
  LOG(INFO) << "--- CONFIGURATION PHASE ---";
  try {
    Config cfg;
    LOG(DEBUG) << "Config object created";
    cfg.parseFile(std::string(path));
    LOG(INFO) << "Configuration file parsed successfully";

    // Validate parsed configuration
    cfg.validate();
    LOG(INFO) << "Configuration validated successfully";

    // Build Server objects from validated config
    servers = cfg.getServers();
    LOG(INFO) << "Server objects built: " << servers.size() << " server(s)";

    // If no valid listen directives were found, print error and exit
    if (servers.empty()) {
      LOG(ERROR)
          << "Error: no valid 'listen' directives found in configuration; "
          << "please specify at least one valid 'listen <port>';";
      return EXIT_FAILURE;
    }

  } catch (const std::exception &e) {
    LOG(ERROR) << std::string("Error: could not read/parse/validate config: ") +
                      e.what();
    return EXIT_FAILURE;
  }

  // ===== SERVER INITIALIZATION =====
  // Initialize sockets and prepare for accepting connections
  LOG(INFO) << "--- SERVER INITIALIZATION PHASE ---";
  ServerManager sm;
  try {
    sm.initServers(servers);
    LOG(INFO) << "All servers initialized and ready to accept connections";
  } catch (const std::exception &e) {
    LOG(ERROR) << e.what();
    return EXIT_FAILURE;
  } catch (...) {
    LOG(ERROR) << "Unknown error while initializing Server";
    return EXIT_FAILURE;
  }

  // ===== EVENT LOOP =====
  // Ignore SIGPIPE to prevent crashes when clients disconnect
  LOG(INFO) << "--- EVENT LOOP PHASE ---";
  signal(SIGPIPE, SIG_IGN);
  LOG(DEBUG) << "SIGPIPE signal ignored";

  LOG(INFO) << "=================================================";
  LOG(INFO) << "  WebServer Ready - Entering Event Loop";
  LOG(INFO) << "=================================================";

  // Enter main event loop (epoll-based I/O multiplexing)
  return sm.run();
}
