#include "IHandler.hpp"

int IHandler::getMonitorFd() const {
  return -1;
}

bool IHandler::checkTimeout(Connection& conn) {
  (void)conn;
  return false;
}
