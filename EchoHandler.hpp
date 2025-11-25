#pragma once

#include "IHandler.hpp"

// EchoHandler is a debug/test handler that echoes the request body back.
// It's not used in production routing but can be instantiated manually for
// testing.
class EchoHandler : public IHandler {
 public:
  EchoHandler();
  virtual ~EchoHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);
};
