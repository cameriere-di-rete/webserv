#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Config.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"

int main(int argc, char** argv) {
  const char* path = (argc > 1) ? argv[1] : "./conf/default.conf";
  LOG(INFO) << "Using configuration file: " << path;

  ServerManager sm;
  try {
    sm.setupSignalHandlers();

    Config cfg;
    cfg.parseFile(std::string(path));
    LOG(INFO) << "Configuration file parsed successfully";

    cfg.debug();

    std::vector<Server> servers = cfg.getServers();
    sm.initServers(servers);
    LOG(INFO) << "All servers initialized and ready to accept connections";

    return sm.run();
  } catch (const std::exception& e) {
    LOG(ERROR) << std::string("Error in config or server initialization: ") +
                      e.what();
    return EXIT_FAILURE;
  } catch (...) {
    LOG(ERROR) << "Unknown error while initializing Server";
    return EXIT_FAILURE;
  }
}
