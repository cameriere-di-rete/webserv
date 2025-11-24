#pragma once

#include "IHandler.hpp"

class PostHandler : public IHandler {
 public:
  PostHandler();
  virtual ~PostHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);
};
