#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Config.hpp"
#include "DeleteHandler.hpp"
#include "HandlerRegistry.hpp"
#include "HeadHandler.hpp"
#include "Logger.hpp"
#include "PostHandler.hpp"
#include "PutHandler.hpp"
#include "ServerManager.hpp"
#include "StaticFileHandler.hpp"
#include "utils.hpp"

// Register all handler prototypes with the registry.
// Handlers are checked in registration order; the first matching handler is used.
// File-based handlers (GET/HEAD) are registered first because they have more
// specific matching criteria (method + resolved path). Method-only handlers
// (POST/PUT/DELETE) are registered after, serving as fallbacks for those methods.
static void registerHandlers() {
  HandlerRegistry& registry = HandlerRegistry::instance();

  // File-based handlers: require method + valid resolved path
  registry.registerHandler(new StaticFileHandler());  // GET + file
  registry.registerHandler(new HeadHandler());        // HEAD + file

  // Method-only handlers: match on HTTP method
  registry.registerHandler(new PostHandler());    // POST
  registry.registerHandler(new PutHandler());     // PUT
  registry.registerHandler(new DeleteHandler());  // DELETE
}

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

  // Register all request handlers
  registerHandlers();

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
    LOG(ERROR) << "Error in config or server initialization: " << e.what();
    return EXIT_FAILURE;
  } catch (...) {
    LOG(ERROR) << "Unknown error while initializing Server";
    return EXIT_FAILURE;
  }
}
