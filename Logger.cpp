#include "Logger.hpp"
#include <ctime>
#include <iostream>
#include <sstream>

Logger::LogLevel Logger::_level = Logger::INFO;

Logger::Logger() {}

Logger::Logger(const Logger &other) { (void)other; }

Logger &Logger::operator=(const Logger &other) {
  (void)other;
  return *this;
}

void Logger::setLevel(LogLevel level) { _level = level; }

std::string Logger::getCurrentTime() {
  time_t now = time(0);
  struct tm *timeinfo = localtime(&now);
  char buffer[20];
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

void Logger::debug(const std::string &message) { log(DEBUG, message); }

void Logger::info(const std::string &message) { log(INFO, message); }

void Logger::error(const std::string &message) { log(ERROR, message); }
