#pragma once

#include <ctime>
#include <set>
#include <string>

#include "IHandler.hpp"

class Connection;

class CgiHandler : public IHandler {
 public:
  CgiHandler(const std::string& script_path,
             const std::set<std::string>& allowed_extensions);
  virtual ~CgiHandler();

  virtual HandlerResult start(Connection& conn);
  virtual HandlerResult resume(Connection& conn);
  virtual int getMonitorFd() const;
  virtual bool checkTimeout(Connection& conn);

 private:
  void setupEnvironment(Connection& conn);
  void cleanupProcess();
  HandlerResult readCgiOutput(Connection& conn);
  HandlerResult parseOutput(Connection& conn, const std::string& data);
  std::string getInterpreter(const std::string& path);
  bool validateScriptPath(const std::string& path, std::string& error_msg);
  bool isAllowedExtension(const std::string& path);
  bool isPathTraversalSafe(const std::string& path);

  std::string script_path_;
  std::set<std::string> allowed_extensions_;
  int script_pid_;
  int pipe_read_fd_;
  int pipe_write_fd_;
  bool process_started_;
  bool headers_parsed_;
  std::string remaining_data_;
  std::string accumulated_output_;
  time_t start_time_;
};
