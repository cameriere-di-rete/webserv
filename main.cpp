#include "Config.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "signals.hpp"

int main(int argc, char **argv) {
  const char *path = (argc > 1) ? argv[1] : "./webserv.conf";

  std::vector<int> ports;

  try {
    Config cfg;
    cfg.parseFile(std::string(path));
    BlockNode root = cfg.getRoot();

    // DEBUG: dumpConfig(root);
    dumpConfig(root);

    // Search for server blocks and extract 'listen' directives
    for (size_t i = 0; i < root.sub_blocks.size(); ++i) {
      const BlockNode &srv = root.sub_blocks[i];
      if (srv.type == "server") {
        for (size_t j = 0; j < srv.directives.size(); ++j) {
          const DirectiveNode &d = srv.directives[j];
          if (d.name == "listen" && d.args.size() > 0) {
            // Expect formats like '8080' or '0.0.0.0:8080'
            std::string a = d.args[0];
            size_t pos = a.find(':');
            std::string portstr =
                (pos == std::string::npos) ? a : a.substr(pos + 1);
            int p = std::atoi(portstr.c_str());
            if (p > 0) {
              ports.push_back(p);
            }
          }
        }
      }
    }

    // If no valid listen directives were found, print error and exit
    if (ports.empty()) {
      LOG(ERROR)
          << "Error: no valid 'listen' directives found in configuration; "
          << "please specify at least one valid 'listen <port>;";
      return EXIT_FAILURE;
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << std::string("Error: could not read/parse config: ") +
                      e.what();
    return EXIT_FAILURE;
  }

  ServerManager sm;
  try {
    sm.initServers(ports);
  } catch (const std::exception &e) {
    LOG(ERROR) << e.what();
    return EXIT_FAILURE;
  } catch (...) {
    LOG(ERROR) << "Unknown error while initializing Server";
    return EXIT_FAILURE;
  }

  /* install signal handlers for graceful shutdown and ignore SIGPIPE */
  setup_signal_handlers();

  return sm.run();
}
