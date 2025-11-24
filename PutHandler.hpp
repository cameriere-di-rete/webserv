#pragma once

#include "IHandler.hpp"

class PutHandler : public IHandler {
 public:
  PutHandler();
  virtual ~PutHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);
};
