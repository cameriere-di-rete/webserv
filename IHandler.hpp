#pragma once

class Connection;
class Request;
class Location;

enum HandlerResult { HR_DONE = 0, HR_WOULD_BLOCK = 1, HR_ERROR = -1 };

class IHandler {
 public:
  virtual ~IHandler() {}
  virtual HandlerResult start(Connection& conn) = 0;
  virtual HandlerResult resume(Connection& conn) = 0;
};
