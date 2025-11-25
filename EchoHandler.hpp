#pragma once

#include "IHandler.hpp"

class EchoHandler : public IHandler {
 public:
  EchoHandler();
  virtual ~EchoHandler();

  // IHandler interface
  virtual bool canHandle(const HandlerContext& ctx) const;
  virtual IHandler* create(const HandlerContext& ctx) const;
  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);
};
