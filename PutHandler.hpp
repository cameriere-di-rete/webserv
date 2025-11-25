#pragma once

#include "IHandler.hpp"

class PutHandler : public IHandler {
 public:
  PutHandler();
  virtual ~PutHandler();

  // IHandler interface
  virtual bool canHandle(const HandlerContext& ctx) const;
  virtual IHandler* create(const HandlerContext& ctx) const;
  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);
};
