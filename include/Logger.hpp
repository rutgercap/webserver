#pragma once

#include <errno.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>

#define LOG_DEST "logs/"

enum ELogLevel { INFO, ERROR };

enum ELogVerbosity { SILENT, MEDIUM, VERBOSE };

#define VERBOSITY MEDIUM

/**
 * Singleton logger
 * Will automatically open file in LOG_DEST and append to it
 * Prints to file in LOG_DEST and stdout / stderr
 * Usage:
 * 		Logger& logger = Logger::getInstance();
 * 		logger.log("example log");
 *
 * 		OR
 *
 * 		Logger::getInstance().log("Example");
 */
class Logger {
 public:
  // Retrieves the logger instance
  static Logger &getInstance();

  void log(std::string const &message);
  void error(std::string const &message);
  void debug(std::string const &message);

  void log(std::string const &message, ELogVerbosity lvl);
  void error(std::string const &message, ELogVerbosity lvl);
  void debug(std::string const &message, ELogVerbosity lvl);

 protected:
  // Constructors are protected since they shouldn't be called by any other
  // class or file
  Logger();
  ~Logger();

  // Singleton, this is the actual instance of the logger
  static Logger *_logger;

 private:
  // Logging file
  std::ofstream _file;
  pid_t _parentPid;

  std::string getTimeStamp() const;
  std::string getPid() const;

  void logToConsole(std::string const &levelMsg, ELogLevel level,
                    std::string const &message);
  void logToFile(std::string const &levelMsg, std::string const &message);

  // Gets if file is open
  bool isFileOpen() const;

  // Prints restart message to log
  void restart();
};
