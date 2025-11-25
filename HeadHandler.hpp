#pragma once

#include <string>

#include "IHandler.hpp"

class HeadHandler : public IHandler {
 public:
  // Default constructor for prototype registration
  HeadHandler();
  explicit HeadHandler(const std::string& path);
  virtual ~HeadHandler();

  // IHandler interface
  virtual bool canHandle(const HandlerContext& ctx) const;
  virtual IHandler* create(const HandlerContext& ctx) const;
  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);

 private:
  std::string file_path_;
};
