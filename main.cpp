#include "ServerManager.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>

int main(void) {
  std::vector<int> ports;
  ports.push_back(8080);
  ports.push_back(9090);

  ServerManager sm;
  try {
    sm.initServers(ports);
  } catch (const std::exception &e) {
    return error(e.what());
  } catch (...) {
    return error("Unknown error while initiating Server");
  }

  /* ignore SIGPIPE so a broken client won't kill the process when we write */
  signal(SIGPIPE, SIG_IGN);

  return sm.run();
}
