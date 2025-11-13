#include "Config.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char **argv) {
  const char *path = (argc > 1) ? argv[1] : "./conf/default.conf";
  LOG(INFO) << "Using configuration file: " << path;

  ServerManager sm;

  try {
    Config cfg;
    cfg.parseFile(std::string(path));
    LOG(INFO) << "Configuration file parsed successfully";

    cfg.debug();

    sm.initServers(cfg.getServers());
    LOG(INFO) << "All servers initialized and ready to accept connections";
  } catch (const std::exception &e) {
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
