#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Config.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"
#include "utils.hpp"

int main(int argc, char** argv) {
  // run `./webserv -l:N` to choose the log level
  // 0 = DEBUG, 1 = INFO, 2 = ERROR

  std::string path;
  int logLevel;

  // collect path and log level from argv
  try {
    processArgs(argc, argv, path, logLevel);
  } catch (const std::exception& e) {
    LOG(ERROR) << "Error processing command-line arguments: " << e.what();
    return EXIT_FAILURE;
  } catch (...) {
    LOG(ERROR) << "Unknown error while processing command-line arguments";
    return EXIT_FAILURE;
  }

  Logger::setLevel(static_cast<Logger::LogLevel>(logLevel));

  ServerManager sm;
  try {
    sm.setupSignalHandlers();

    Config cfg;
    cfg.parseFile(std::string(path));

    cfg.debug();

    std::vector<Server> servers = cfg.getServers();
    sm.initServers(servers);
    LOG(INFO) << "All servers initialized and ready to accept connections";

    return sm.run();
  } catch (...) {
    return EXIT_FAILURE;
  }
}
