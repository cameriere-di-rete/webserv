#pragma once

#include "IHandler.hpp"

class PostUploadHandler : public IHandler {
 public:
  PostUploadHandler();
  virtual ~PostUploadHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);
};