#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Config.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"

int parseLogLevelFlag(const std::string& arg) {
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

// Parse program arguments and fill `path` and `logLevel`.
// This was moved out of main to keep main shorter and clearer.
void processArgs(int argc, char** argv, std::string& path, int& logLevel) {
  // Defaults
  logLevel = 1;  // Default: INFO

  bool logFlagSet = false;

  // Parse arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    int level = parseLogLevelFlag(arg);

    if (level >= 0) {
      if (!logFlagSet) {
        logLevel = level;
        logFlagSet = true;
      }
    } else {
      if (path.empty()) {
        path = arg;
      } else {
        throw std::runtime_error("Error: multiple config file paths provided");
      }
    }
  }
  if (path.empty()) {
    path = "./webserv.conf";
  }
}

int main(int argc, char** argv) {
  // run `./webserv -l:N` to choose the log level
  // 0 = DEBUG, 1 = INFO, 2 = ERROR

  std::string path;
  int logLevel;

  // collect path and log level from argv
  processArgs(argc, argv, path, logLevel);

  Logger::setLevel(static_cast<Logger::LogLevel>(logLevel));

  ServerManager sm;

  try {
    Config cfg;
    cfg.parseFile(std::string(path));
    LOG(INFO) << "Configuration file parsed successfully";

    cfg.debug();

    std::vector<Server> servers = cfg.getServers();
    sm.initServers(servers);
    LOG(INFO) << "All servers initialized and ready to accept connections";
  } catch (const std::exception& e) {
    LOG(ERROR) << std::string("Error in config or server initialization: ") +
                      e.what();
    return EXIT_FAILURE;
  } catch (...) {
    LOG(ERROR) << "Unknown error while initializing Server";
    return EXIT_FAILURE;
  }

  signal(SIGPIPE, SIG_IGN);

  return sm.run();
}
