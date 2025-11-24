#pragma once

#include "IHandler.hpp"

class DeleteHandler : public IHandler {
 public:
  DeleteHandler();
  virtual ~DeleteHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);
};
