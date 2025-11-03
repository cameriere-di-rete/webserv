#pragma once

#include <string>

class Logger {
public:
  enum LogLevel { DEBUG, INFO, ERROR };

private:
  Logger();
  Logger(const Logger &other);
  Logger &operator=(const Logger &other);

  static LogLevel _level;

  static std::string getCurrentTime();
  static std::string levelToString(LogLevel level);

public:
  static void setLevel(LogLevel level);
  static void log(LogLevel level, const std::string &message);
  static void debug(const std::string &message);
  static void info(const std::string &message);
  static void error(const std::string &message);
};
