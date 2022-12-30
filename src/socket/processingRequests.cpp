#include <Multiplexer.hpp>

#define MAX_REQUEST_SIZE 1000000
#define REQPARSER_BUFF_SIZE 1024

static int readFromClientFd(std::string *result, const int clientFd) {
  char buffer[REQPARSER_BUFF_SIZE + 1];
  memset(buffer, 0, REQPARSER_BUFF_SIZE + 1);

  ssize_t bytesReceived = read(clientFd, buffer, REQPARSER_BUFF_SIZE);
  if (bytesReceived == -1) {
    Logger::getInstance().error(
        "[SOCKET]: read failed: " + std::to_string(clientFd) + ": " +
        std::string(strerror(errno)));
    return -1;
  }
  *result = std::string(buffer, bytesReceived);
  Logger::getInstance().log(
      "[READING] Multiplexer: Read " + std::to_string(bytesReceived), VERBOSE);
  return bytesReceived;
}

/**
 * @return host without port OR if invalid host -> empty string
 */
static std::string getHostWithoutPort(HttpRequest *request) {
  std::string host = request->getHeader("Host");
  if (host.length() == 0) {
    return "";
  }
  size_t semiCol = host.find(':');
  if (semiCol != std::string::npos) {
    host = host.substr(0, semiCol);
  }
  return host;
}

/* Checks which servers are listening to the host from the request*/
static Server *matchBasedOnHost(std::vector<Server> &allServers,
                                const std::string &host) {
  for (std::vector<Server>::iterator it = allServers.begin();
       it != allServers.end(); ++it) {
    if (it->getServerName() == host) {
      return &*it;
    }
  }
  return &allServers.front();
}

/**
 * Finds server that matches the sent request, then adds it to matching client
 */
Server *Socket::_matchRequestToServer(HttpRequest *request) {
  std::string hostWithoutPort = getHostWithoutPort(request);
  return matchBasedOnHost(_servers, hostWithoutPort);
}

std::string Socket::_addRawRequest(const int &fd,
                                   std::string const &rawRequest) {
  if (_unfinishedRequest.count(fd)) {
    _unfinishedRequest[fd].rawRequest += rawRequest;
  } else {
    _unfinishedRequest[fd].rawRequest = rawRequest;
  }
  return _unfinishedRequest[fd].rawRequest;
}

void Socket::_addUnfinishedRequest(const int &fd, HttpRequest *request,
                                   Server *match) {
  UnfinishedRequest dest = _unfinishedRequest.at(fd);
  dest.rawRequest = "";
  dest.status = RequestStatus::UNFINISHED_REQUEST;
  dest.request = request;
  dest.server = match;
}

void Socket::_removeUnfinishedRequest(const int &fd) {
  _unfinishedRequest.erase(fd);
}

static bool isRawRequestFinished(const std::string &rawRequest) {
  size_t endOfHeader = rawRequest.find("\r\n\r\n");
  if (endOfHeader == std::string::npos) {
    return false;
  }
  return true;
}

static bool isRequestFinished(const HttpRequest &request) {
  int contentLength = request.getIntHeader("Content-Length");
  if (request.getBody().length() < contentLength) {
    return false;
  }
  return true;
}

RequestStatus Socket::getRequestStatus(const int &clientFd) const {
  if (_unfinishedRequest.count(clientFd) == 0) {
    return RequestStatus::NONE;
  }
  return _unfinishedRequest.at(clientFd).status;
}

bool isRequestFinished(const HttpRequest &request, const int &maxSize) {
  if (request.getMethod() != EHttpMethods::POST) {
    return true;
  }
  if (!request.hasHeader("Content-Length")) {
    return true;
  }
  int contentLength = request.getIntHeader("Content-Length");
  if (contentLength < 0) {
    return true;
  }
  if (request.getBody().length() > maxSize) {
    return true;
  }
  if (request.getBody().length() < contentLength) {
    return false;
  }
  return true;
}

void Socket::_processRawRequest(const int &fd, const std::string &rawRequest) {
  std::string fullRequest = _addRawRequest(fd, rawRequest);
  if (isRawRequestFinished(fullRequest) == false) {
    return;
  }
  HttpRequest *request = RequestParser::parseHeader(fullRequest);
  // TODO: check if this is correct
  if (request->getStatus() != HTTPStatusCode::NOT_SET) {
    _addBadRequestToClient(fd, request->getStatus());
    _removeUnfinishedRequest(fd);
  }
  Server *match = _matchRequestToServer(request);
  if (isRequestFinished(*request, match->getMaxBody())) {
    _addRequestToClient(fd, request, match);
    _removeUnfinishedRequest(fd);
  } else {
    _addUnfinishedRequest(fd, request, match);
  }
}

int Socket::processUnfinishedRequest(const int &fd,
                                     const std::string &rawRequest) {
  HttpRequest *request = _unfinishedRequest[fd].request;
  Server *match = _unfinishedRequest[fd].server;

  request->addBody(rawRequest);
  if (isRequestFinished(*request)) {
    _addRequestToClient(fd, request, match);
    _removeUnfinishedRequest(fd);
  }
}

int Socket::processRequest(int const &fd) {
  std::string rawRequest;

  Logger::getInstance().log("[SOCKET] Starting to read from client", VERBOSE);
  int bytesRead = readFromClientFd(&rawRequest, fd);
  if (bytesRead < 0) {
    _addBadRequestToClient(fd, HTTPStatusCode::INTERNAL_SERVER_ERROR);
    return 1;
  }
  if (getRequestStatus(fd) != RequestStatus::UNFINISHED_REQUEST) {
    _processRawRequest(fd, rawRequest);
  } else {
    processUnfinishedRequest(fd, rawRequest);
  }
  return 0;
}
