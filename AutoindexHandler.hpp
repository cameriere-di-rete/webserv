#pragma once

#include <string>

#include "IHandler.hpp"

class AutoindexHandler : public IHandler {
 public:
  explicit AutoindexHandler(const std::string& dirpath);
  virtual ~AutoindexHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);

 private:
  std::string dirpath_;
};
