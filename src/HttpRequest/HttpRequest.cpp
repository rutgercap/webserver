#include <HttpRequest.hpp>

#include "Logger.hpp"

/*
 *  Canonical
 */
HttpRequest::HttpRequest(HttpHeaderData const &data)
    : HttpMessage(data.httpVersion, data.headers, data.body),
      _method(data.method),
      _uri(data.url) {}

HttpRequest::HttpRequest() {}

HttpRequest::HttpRequest(const HttpRequest &obj) : HttpMessage(obj) {
  _method = obj._method;
  _uri = obj._uri;
}

HttpRequest::~HttpRequest() {}

/*
 * Protected methods
 */
FileType HttpRequest::_getFileType(const std::string &path) const {
  Logger &logger = Logger::getInstance();

  struct stat buffer;

  if (stat(path.c_str(), &buffer) == -1) {
    logger.log(
        "[RESPONSE-BUILDING]: _fileExists -> File doesn't exist "
        "(" +
            path + ")",
        VERBOSE);
    return FileType::NOT_FOUND;
  }
  if (buffer.st_mode & S_IFDIR) {
    logger.log(
        "[RESPONSE-BUILDING]: _fileExists -> File is a directory "
        "(" +
            path + ")",
        VERBOSE);
    return FileType::DIR;
  }
  if (buffer.st_mode & S_IFREG) {
    logger.log(
        "[RESPONSE-BUILDING]: _fileExists -> File is a regular "
        "file "
        "(" +
            path + ")",
        VERBOSE);
    return FileType::FILE;
  }
  logger.log(
      "[RESPONSE-BUILDING]: _fileExists -> File is something else "
      "(" +
          path + ")",
      VERBOSE);
  logger.error("WARNING: logic probably not implemented yet");
  return FileType::NOT_FOUND;
}

static std::string constructPath(const std::string &root,
                                 const std::string &uri) {
  std::string fullPath;

  // eg: root = "root/", _uri = "/index.html"
  // => full_path = "root/index.html"
  if (!root.empty()) {
    return root + uri;
  } else {
    return uri;
  }
}

static bool isMethodAllowed(const std::map<EHttpMethods, bool> allowedMethods,
                            EHttpMethods method) {
  for (std::map<EHttpMethods, bool>::const_iterator it = allowedMethods.begin();
       it != allowedMethods.end(); it++) {
    if (it->first == method) {
      return it->second;
    }
  }
  return false;
}

static bool isCgiRequest(std::string path) {
  if (path.substr(path.length() - 3) == ".py") {
    return true;
  }
  return false;
}

HttpResponse HttpRequest::executeRequest(const Server &server) {
  const Route &routeOfResponse = server.getRoute(HttpRequest::_uri);
  const std::string path = constructPath(routeOfResponse.rootDirectory, _uri);

  if (isMethodAllowed(routeOfResponse.allowedMethods, _method) == false) {
    Logger::getInstance().error("Method for request not allowed");
    return HttpResponse(HTTPStatusCode::METHOD_NOT_ALLOWED);
  }

  FileType type = HttpRequest::_getFileType(path);
  if (type == FileType::NOT_FOUND) {
    // hier Oswin troep
    return _errorResponse(HTTPStatusCode::NOT_FOUND, routeOfResponse);
  }

  if (isCgiRequest(path)) {
    return _handleCgiRequest(path, routeOfResponse);
  }
  return _handleFileRequest(path, routeOfResponse, type);
}

// TODO: add redirection support, with return ? 301 and 302
// auto index
// - if auto index is on and the path is a directory, look for index files and
// return it
// - if auto index is off and the path is a directory, return 403
HttpResponse HttpRequest::_handleCgiRequest(std::string const &path,
                                            Route const &route) const {
  Logger &logger = Logger::getInstance();
  std::map<std::string, std::string> headers;
  std::string body;

  logger.log("starting cgi request", VERBOSE);
  if (!route.cgiEnabled) {
    logger.error("CGI not allowed on this route");
    return HttpResponse(HTTPStatusCode::METHOD_NOT_ALLOWED);
  }
  std::string cgiBody = _body;
  if (route.maxBodySize != 0 && _body.length() > route.maxBodySize) {
    cgiBody = _body.substr(0, route.maxBodySize);
  }
  HTTPStatusCode status = CGI::executeFile(&body, &headers, path, cgiBody);
  if (status != HTTPStatusCode::OK) {
    Logger::getInstance().error("Executing cgi: " +
                                getMessageByStatusCode(status));
    return HttpResponse(status);
  }

  logger.log("CGI request successful", VERBOSE);
  return _responseWithBody(headers, body);
}

HttpResponse HttpRequest::_handleFileRequest(std::string const &path,
                                             Route const &route,
                                             const FileType &type) const {
  switch (type) {
    case FileType::DIR: {
      if (_isTypeAccepted() == false) {
        return _errorResponse(HTTPStatusCode::NOT_ACCEPTABLE, route);
      }
      std::vector<std::string> possiblePaths =
          _getPossiblePaths(path, route.indexFiles);
      for (std::vector<std::string>::const_iterator it = possiblePaths.begin();
           it != possiblePaths.end(); ++it) {
        if (_getFileType(*it) == FileType::FILE) {
          return _responseWithFile(*it, HTTPStatusCode::OK);
        }
      }
      return HttpResponse(HTTPStatusCode::NOT_FOUND);
    }
    case FileType::FILE: {
      if (_isTypeAccepted() == false) {
        return _errorResponse(HTTPStatusCode::NOT_ACCEPTABLE, route);
      }
      return _responseWithFile(path, HTTPStatusCode::OK);
    }
    case FileType::NOT_FOUND: {
      return _errorResponse(HTTPStatusCode::NOT_FOUND, route);
    }
    default: {
      return _errorResponse(HTTPStatusCode::INTERNAL_SERVER_ERROR, route);
    }
  }
}

static std::string getFileSize(std::ifstream &file) {
  std::stringstream ss;

  file.seekg(0, std::ios::end);
  ss << file.tellg();
  file.seekg(0, std::ios::beg);
  return ss.str();
}

static std::string fileToStr(std::ifstream &file) {
  file.seekg(0, std::ios::beg);

  std::string body((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
  return body;
}

/*
 * Get the file type of a file
 * @param path The path to the file to check
 * @return The content type of the file
 */
static std::string getContentType(const std::string &path) {
  std::map<std::string, std::string> file_types;

  file_types["html"] = "text/html";
  file_types["css"] = "text/css";
  file_types["js"] = "application/javascript";
  file_types["png"] = "image/png";

  std::string file_extension = path.substr(path.find_last_of(".") + 1);
  if (file_types.find(file_extension) != file_types.end()) {
    return file_types[file_extension];
  }
  return "text/plain";
}

HttpResponse HttpRequest::_responseWithFile(std::string const &path,
                                            HTTPStatusCode statusCode) const {
  std::ifstream file(path.c_str());
  if (!file.is_open()) {
    return HttpResponse(HTTPStatusCode::INTERNAL_SERVER_ERROR);
  }
  HttpResponse response(statusCode);
  std::string test = fileToStr(file);
  response.setBody(fileToStr(file));
  response.setHeader("Content-Length", getFileSize(file));
  response.setHeader("Content-Type", getContentType(path));
  return response;
}

HttpResponse HttpRequest::_responseWithBody(
    std::map<std::string, std::string> headers, std::string body) const {
  HttpResponse response(HTTPStatusCode::OK);
  response.setBody(body);
  for (std::map<std::string, std::string>::const_iterator it = headers.begin();
       it != headers.end(); ++it) {
    response.setHeader(it->first, it->second);
  }
  return response;
}

std::vector<std::string> HttpRequest::_getPossiblePaths(
    const std::string &path, const std::vector<std::string> &indexFiles) const {
  std::vector<std::string> possiblePaths;

  for (std::vector<std::string>::const_iterator it = indexFiles.begin();
       it != indexFiles.end(); ++it) {
    possiblePaths.push_back(path + "/" + *it);
  }

  return possiblePaths;
}

bool HttpRequest::_isTypeAccepted() const {
  // All types the server accepts
  const std::array<std::string, 4> typesToAccept = {
      "text/html", "text/css", "application/javascript", "image/jpg"};
  const std::string acceptHeader = getHeader("Accept");
  std::vector<std::string> acceptedTypes;

  if (acceptHeader.empty() || acceptHeader.find("*/*") != std::string::npos) {
    return true;
  } else {
    // TODO: replace with splitheader function
    std::size_t pos = 0;
    std::size_t prev = 0;
    while ((pos = acceptHeader.find(',', prev)) != std::string::npos) {
      acceptedTypes.push_back(acceptHeader.substr(prev, pos - prev));
      prev = pos + 1;
    }
    acceptedTypes.push_back(acceptHeader.substr(prev));
  }

  for (size_t i = 0; i < typesToAccept.size(); i++) {
    if (std::find(acceptedTypes.begin(), acceptedTypes.end(),
                  typesToAccept[i]) != acceptedTypes.end()) {
      return true;
    }
  }
  return false;
}

HttpResponse HttpRequest::_errorResponse(HTTPStatusCode const &statusCode,
                                         Route const &route) const {
  (void)route;
  Logger::getInstance().error(
      "Making error with message:" + getMessageByStatusCode(statusCode),
      VERBOSE);
  return HttpResponse(statusCode);
}

HttpRequest *HttpRequest::operator+(const HttpRequest &chunk) {
  if (chunk._statusCode != HTTPStatusCode::NOT_SET) {
    delete this;
    return new BadRequest(HTTPStatusCode::BAD_REQUEST);
  }
  _body += chunk._body;
  _headers.insert(std::begin(chunk._headers), std::end(chunk._headers));
  return this;
}

void HttpRequest::unChunkBody() { _body = unChunk(_body); }

/**
 * Determines whether a given HTTP request uses chunked transfer encoding.
 */
bool HttpRequest::isFirstChunk() {
  return "chunked" == getHeader("Transfer-Encoding");
}
