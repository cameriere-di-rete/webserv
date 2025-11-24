#pragma once

#include <string>

#include "IHandler.hpp"

class HeadHandler : public IHandler {
 public:
  explicit HeadHandler(const std::string& path);
  virtual ~HeadHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);

 private:
  std::string file_path_;
};
