#pragma once

#include "IHandler.hpp"

class EchoHandler : public IHandler {
 public:
  EchoHandler();
  virtual ~EchoHandler();

  virtual HandlerResult start(Connection& conn, const Location& loc);
  virtual HandlerResult resume(Connection& conn);
};
