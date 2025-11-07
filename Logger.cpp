#include "Logger.hpp"
#include <cstring>
#include <ctime>
#include <iostream>

#include <sstream>

// Logger instance implementation used as a temporary RAII stream object
// constructed by the LOG(...) macro.

Logger::Logger(LogLevel level, const char *file, int line)
    : _msgLevel(level), _file(file), _line(line) {}

Logger::~Logger() {
  std::ostringstream o;
  o << "(" << _file << ":" << _line << ") " << _stream.str();
  Logger::log(_msgLevel, o.str());
}

std::ostringstream &Logger::stream() {
  return _stream;
}

// Map numeric LOG_LEVEL macro to the LogLevel enum. Define LOG_LEVEL in
// constants.hpp (or via -DLOG_LEVEL=N). If LOG_LEVEL is not defined here we
// default to INFO.
#ifndef LOG_LEVEL
#define LOG_LEVEL Logger::INFO
#endif

Logger::LogLevel Logger::_level = static_cast<Logger::LogLevel>(
    (LOG_LEVEL >= Logger::DEBUG && LOG_LEVEL <= Logger::ERROR) ? LOG_LEVEL
                                                               : Logger::INFO);

void Logger::setLevel(LogLevel level) {
  _level = level;
}

std::string Logger::getCurrentTime() {
  time_t now = time(0);
  struct tm *timeinfo = localtime(&now);
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return std::string(buffer);
}

std::string Logger::levelToString(LogLevel level) {
  switch (level) {
  case DEBUG:
    return "DEBUG";
  case INFO:
    return "INFO";
  case ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

void Logger::log(LogLevel level, const std::string &message) {
  if (level < _level) {
    return;
  }

  std::cout << "[" << getCurrentTime() << "] [" << levelToString(level) << "] "
            << message << std::endl;
}

void Logger::debug(const std::string &message) {
  log(DEBUG, message);
}

void Logger::info(const std::string &message) {
  log(INFO, message);
}

void Logger::error(const std::string &message) {
  log(ERROR, message);
}

void Logger::printStartupLevel() {
  std::cout << "Effective log level: " << levelToString(_level) << std::endl;
}
