#include "Config.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

int parseLogLevelFlag(const std::string &arg) {
  // Guard 1: Wrong length
  if (arg.length() != 4) {
    return -1;
  }

  // Guard 2: Wrong prefix
  if (arg.compare(0, 3, "-l:") != 0) {
    return -1;
  }

  // Guard 3: Invalid value
  char level = arg[3];
  if (level < '0' || level > '2') {
    return -1;
  }

  // All good
  return level - '0';
}

int main(int argc, char **argv) {

  // 0 = DEBUG, 1 = INFO, 2 = ERROR
  // run `./webserv -l:N` to choose the log level

  std::string path = "./webserv.conf";
  int logLevel = 1; // Default: INFO

  // Parse arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    int level = parseLogLevelFlag(arg);

    if (level >= 0) {
      logLevel = level;
    } else {
      path = arg; // Overwrite if passed
    }
  }

  Logger::setLevel(static_cast<Logger::LogLevel>(logLevel));

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

  /* ignore SIGPIPE so a broken client won't kill the process when we write */
  signal(SIGPIPE, SIG_IGN);

  return sm.run();
}
