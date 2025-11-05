#include "Config.hpp"
#include "ServerManager.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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
            if (p > 0)
              ports.push_back(p);
          }
        }
      }
    }

    // If no valid listen directives were found, print error and exit
    if (ports.empty()) {
      return error(
          "Error: no valid 'listen' directives found in configuration; "
          "please specify at least one valid 'listen <port>;'");
    }
  } catch (const std::exception &e) {
    return error(std::string("Error: could not read/parse config: ") +
                 e.what());
  }

  ServerManager sm;
  try {
    sm.initServers(ports);
  } catch (const std::exception &e) {
    return error(e.what());
  } catch (...) {
    return error("Unknown error while initializing Server");
  }

  /* ignore SIGPIPE so a broken client won't kill the process when we write */
  signal(SIGPIPE, SIG_IGN);

  return sm.run();
}
