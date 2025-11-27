#pragma once

#include <string>

#include "IHandler.hpp"

class Connection;

class CgiHandler : public IHandler {
 public:
  explicit CgiHandler(const std::string& script_path);
  virtual ~CgiHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);
  virtual int getMonitorFd() const;

 private:
  void setupEnvironment(Connection& conn);
  void cleanupProcess();
  HandlerResult readCgiOutput(Connection& conn);
  HandlerResult parseOutput(Connection& conn, const std::string& data);

  std::string script_path_;
  int script_pid_;
  int pipe_read_fd_;
  int pipe_write_fd_;
  bool process_started_;
  bool headers_parsed_;
  bool cgi_finished_;
  std::string remaining_data_;
  std::string accumulated_output_;
};
