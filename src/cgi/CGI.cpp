#include <unistd.h>

#include <CGI.hpp>

CGI::CGI() {}

CGI::~CGI() {}

static char *const *strlist(const std::vector<const std::string> &input) {
  char **result = new char *[input.size() + 1];
  std::size_t storage_size = 0;
  for (auto const &s : input) {
    storage_size += s.size() + 1;
  }

  try {
    char *storage = new char[storage_size];
    char *p = storage;
    char **q = result;
    for (auto const &s : input) {
      *q++ = std::strcpy(p, s.c_str());
      p += s.size() + 1;
    }
    *q = nullptr;  // terminate the list

    return result;
  } catch (...) {
    delete[] result;
    throw;
  }
}

static char *const *makeArgv(const std::string &filePath,
                             std::vector<std::string> cgiParams) {
  const char *path = PATH_TO_PYTHON;
  std::vector<const std::string> argVector = {path, filePath.c_str()};
  if (cgiParams.empty()) {
    return strlist(argVector);
  }
  argVector.reserve(argVector.size() + cgiParams.size());
  for (std::vector<std::string>::iterator it = cgiParams.begin();
       it != cgiParams.end(); it++) {
    argVector.push_back(it->c_str());
  }
  return strlist(argVector);
}

/* Child process that executes CGI file */
int CGI::_forkCgiFile(int fd[2], char *const *argv) {
  Logger &logger = Logger::getInstance();

  close(fd[READ]);
  if (dup2(fd[WRITE], STDOUT_FILENO) == -1) {
    logger.error("[EXECUTING] CGI: dup2: " + std::string(strerror(errno)));
    close(fd[WRITE]);
    exit(1);
  }
  if (dup2(fd[WRITE], STDERR_FILENO) == -1) {
    logger.error("[EXECUTING] CGI: dup2: " + std::string(strerror(errno)));
    close(fd[WRITE]);
    exit(1);
  }
  if (execve(PATH_TO_PYTHON, argv, environ)) {
    logger.error("[EXECUTING] CGI: execve: " + std::string(strerror(errno)));
    close(fd[WRITE]);
    exit(1);
  }
  return 1;
}

static int waitForChildProcess(pid_t const &pid) {
  int status = 0;

  if (waitpid(pid, &status, 0) < 0) {
    Logger::getInstance().error("[EXECUTING] waitpid: " +
                                std::string(strerror(errno)));
    return 1;
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return 0;
}

/* Waits for input from child process and puts it in pDest */
static int readFromChildProcess(std::string *pDest, pid_t const &pid, int fd) {
  if (waitForChildProcess(pid) != 0) {
    return 1;
  }

  std::string output;
  std::string buffer[BUFFER_SIZE + 1];
  int bytesRead;

  do {
    bytesRead = read(fd, &buffer, BUFFER_SIZE);
    if (bytesRead == -1) {
      Logger::getInstance().error("[EXECUTING] CGI: read: " +
                                  std::string(strerror(errno)));
      return 1;
    }
    if (bytesRead > 0) {
      output += buffer->c_str();
      buffer->clear();
    }
  } while (bytesRead > 0);
  close(fd);
  *pDest = output;
  return 0;
}

static HTTPStatusCode checkFileAccess(std::string const &filePath) {
  if (access(filePath.c_str(), R_OK)) {
    Logger::getInstance().error("[PREPARING] CGI: access: " +
                                std::string(strerror(errno)));
    switch (errno) {
      case EACCES:
        return HTTPStatusCode::METHOD_NOT_ALLOWED;
      case ENOENT:
        return HTTPStatusCode::NOT_FOUND;
      default:
        return HTTPStatusCode::INTERNAL_SERVER_ERROR;
    }
  }
  return HTTPStatusCode::OK;
}

static std::string trimString(std::string const &string) {
  size_t i = 0;
  while (string[i] == ' ') {
    i++;
  }
  return string.substr(i);
}

static int makeHeaders(std::map<std::string, std::string> *dest,
                       std::vector<std::string> headers) {
  Logger &logger = Logger::getInstance();
  size_t semiColLocation;
  std::string key;
  std::string value;

  for (std::vector<std::string>::const_iterator it = headers.begin();
       it != headers.end(); it++) {
    semiColLocation = it->find(':');
    if (semiColLocation == std::string::npos) {
      logger.error("No semicolon found in header from CGI output");
      return 1;
    }
    key = it->substr(0, semiColLocation);
    value = trimString(it->substr(semiColLocation + 1, it->length()));
    (*dest)[key] = value;
  }
  return 0;
}

static int splitHeaderFromBody(std::string *pBody,
                               std::vector<std::string> *pHeaders,
                               std::string src) {
  Logger &logger = Logger::getInstance();
  size_t endOfHeader = src.find("\n\n");
  if (endOfHeader == std::string::npos) {
    logger.error("Incorrect end of header found -> returning new BadRequest()");
    return 1;
  }
  *pBody = src.substr(endOfHeader + 2);
  *pHeaders = splitHeader(src.substr(0, endOfHeader + 1), "\n");
  return 0;
}

static HTTPStatusCode parseCgiOutput(
    std::string *pBody, std::map<std::string, std::string> *pHeaders,
    std::string &src) {
  std::vector<std::string> headers;
  if (splitHeaderFromBody(pBody, &headers, src)) {
    return HTTPStatusCode::INTERNAL_SERVER_ERROR;
  }
  if (makeHeaders(pHeaders, headers)) {
    pHeaders->clear();
    pBody->clear();
    return HTTPStatusCode::INTERNAL_SERVER_ERROR;
  }
  return HTTPStatusCode::OK;
}

HTTPStatusCode CGI::executeCgi(std::string *pBody,
                               std::map<std::string, std::string> *pHeaders,
                               char *const *argv) {
  Logger &logger = Logger::getInstance();
  std::string buffer;
  int fd[2];
  pid_t pid;

  logger.log("[STARTING] CGI ", VERBOSE);
  if (pipe(fd) == -1) {
    throw std::runtime_error("[PREPARING] CGI: pipe: " +
                             std::string(strerror(errno)));
  }

  pid = fork();
  if (pid == -1) {
    throw std::runtime_error("[PREPARING] CGI: fork: " +
                             std::string(strerror(errno)));
  } else if (pid == CHILD) {
    _forkCgiFile(fd, argv);
  } else {
    close(fd[WRITE]);
    if (readFromChildProcess(&buffer, pid, fd[READ])) {
      return HTTPStatusCode::INTERNAL_SERVER_ERROR;
    }
  }
  return parseCgiOutput(pBody, pHeaders, buffer);
}

HTTPStatusCode CGI::executeFile(std::string *pBody,
                                std::map<std::string, std::string> *pHeaders,
                                const std::vector<std::string> &cgiParams,
                                std::string const &filePath) {
  Logger &logger = Logger::getInstance();

  HTTPStatusCode status = checkFileAccess(filePath);
  if (status != HTTPStatusCode::OK) {
    logger.error("No file access for cgi request");
    return status;
  }

  std::vector<std::string> notConstFuckingVector = cgiParams;
  char *const *argv = makeArgv(filePath, notConstFuckingVector);
  return executeCgi(pBody, pHeaders, argv);
}

HTTPStatusCode CGI::executeFileWithBody(
    std::string *pBody, std::map<std::string, std::string> *pHeaders,
    const std::vector<std::string> &cgiParams, std::string const &filePath,
    std::string const &body) {
  Logger &logger = Logger::getInstance();

  HTTPStatusCode status = checkFileAccess(filePath);
  if (status != HTTPStatusCode::OK) {
    logger.error("No file access for cgi request");
    return status;
  }

  std::vector<std::string> vectorWithBody = cgiParams;
  vectorWithBody.push_back(body);
  char *const *argv = makeArgv(filePath, vectorWithBody);
  return executeCgi(pBody, pHeaders, argv);
}