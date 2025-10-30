#include "ServerManager.hpp"
#include <unistd.h>

ServerManager::ServerManager() : _efd(-1) {}

ServerManager::~ServerManager() {
  if (_efd >= 0) {
    close(_efd);
  }
}

ServerManager::ServerManager(const ServerManager &other) {
  (void)other;
}

ServerManager &ServerManager::operator=(const ServerManager &other) {
  (void)other;
  return *this;
}

void ServerManager::initServers(const std::vector<int> &ports) {};

void ServerManager::acceptConnection(int listen_fd) {};

int ServerManager::run() {
  return 0;
};

void ServerManager::updateEvents(int fd, uint32_t events) {};

// shutdown / cleanup
void ServerManager::shutdown() {};
