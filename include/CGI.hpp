#pragma once

#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <Logger.hpp>
#include <StatusCodes.hpp>
#include <Webserver.hpp>
#include <fstream>
#include <map>
#include <string>

/* Piping and forking variables */
#define CHILD 0
#define READ 0
#define WRITE 1
#define BUFFER_SIZE 1024

#define PATH_TO_PYTHON "/usr/bin/python3"
#define PATH_TO_PHP "/opt/homebrew/Cellar/php/8.1.13/bin/php"

class CGI {
 public:
  /* Can execute file with body */
  static HTTPStatusCode executeFileWithBody(
      std::string *pBody, std::map<std::string, std::string> *pHeaders,
      const std::vector<std::string> &cgiParams, std::string const &filePath,
      std::string const &body);
  /* Or without */
  static HTTPStatusCode executeFile(
      std::string *pBody, std::map<std::string, std::string> *pHeaders,
      const std::vector<std::string> &cgiParams, std::string const &filePath);

 protected:
  static int _forkCgiFile(int fd[2], char *const *argv);

  static HTTPStatusCode executeCgi(std::string *pBody,
                                   std::map<std::string, std::string> *pHeaders,
                                   char *const *argv);

 private:
  CGI();
  ~CGI();
};
