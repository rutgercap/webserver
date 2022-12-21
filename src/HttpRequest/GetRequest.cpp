#include <HttpRequest.hpp>

#define DEFAULT_ERROR_PAGE "root/error_pages/404/index.html"

GetRequest::GetRequest(HttpHeaderData const &data) : HttpRequest(data) {}

GetRequest::GetRequest(const GetRequest &ref) : HttpRequest(ref) {}

GetRequest::~GetRequest() {}

HTTPStatusCode GetRequest::executeRequest(const Server &server) {
  (void)server;
  return HTTPStatusCode::OK;
}

// TODO: add redirection support, with return ? 301 and 302
// auto index
// - if auto index is on and the path is a directory, look for index files and return it
// - if auto index is off and the path is a directory, return 403
static bool isCgiRequest(std::string path) {
  if (path.substr(path.length() - 3) == ".py") {
    return true;
  }
  return false;
}

HttpResponse GetRequest::constructResponse(const Server &server) {
  Route const &routeOfResponse = server.getRoute(HttpRequest::_uri);
  const std::string path = _constructPath(routeOfResponse.rootDirectory);

  if (_isMethodAllowed(routeOfResponse.allowedMethods) == false) {
    return HttpResponse(HTTPStatusCode::METHOD_NOT_ALLOWED);
  }

  if (isCgiRequest(path)) {
    return _handleCgiRequest(path, routeOfResponse);
  }
  return _handleFileRequest(path, routeOfResponse);
}

HttpResponse GetRequest::_handleCgiRequest(std::string const &path,
                                           Route const &route) const {
  std::vector<std::string> headers;
  std::string body;

  HTTPStatusCode status = CGI::executeFile(&body, &headers, path, "");
  if (status != HTTPStatusCode::OK) {
    return _errorResponseWithFile(status, route);
  }
}

HttpResponse GetRequest::_handleFileRequest(std::string const &path,
                                            Route const &route) const {
  switch (_getFileType(path)) {
    case FileType::IS_DIR: {
      if (_typeIsAccepted() == false) {
        return _errorResponseWithFile(HTTPStatusCode::NOT_ACCEPTABLE, route);
      }
      std::vector<std::string> possible_paths =
          _getPossiblePaths(path, route.indexFiles);
      for (std::vector<std::string>::const_iterator it = possible_paths.begin();
           it != possible_paths.end(); ++it) {
        if (_getFileType(*it) == FileType::IS_REG_FILE) {
          return _responseWithFile(*it, HTTPStatusCode::OK);
        }
      }
      return HttpResponse(HTTPStatusCode::NOT_FOUND);
    }
    case FileType::IS_REG_FILE: {
      if (_typeIsAccepted() == false) {
        return _errorResponseWithFile(HTTPStatusCode::NOT_ACCEPTABLE, route);
      }
      return _responseWithFile(path, HTTPStatusCode::OK);
    }
    case FileType::IS_UNKNOWN: {
      return _errorResponseWithFile(HTTPStatusCode::NOT_FOUND, route);
    }
    default: {
      return _errorResponseWithFile(HTTPStatusCode::INTERNAL_SERVER_ERROR,
                                    route);
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

HttpResponse GetRequest::_errorResponseWithFile(
    HTTPStatusCode const &statusCode, Route const &route) const {
  std::string pathToErrorFile = _getErrorPageIndex(route, statusCode);
  return _responseWithFile(pathToErrorFile, statusCode);
}

HttpResponse GetRequest::_responseWithFile(std::string const &path,
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

HttpResponse GetRequest::_responseWithBody(
    std::map<std::string, std::string> headers, std::string body) const {}

bool GetRequest::_typeIsAccepted() const {
  // TYPES_TO_ACCEPT is a vector of strings that contains all the types that the server can accept
  const std::vector<std::string> TYPES_TO_ACCEPT = {"text/html", "text/css", "application/javascript", "image/jpg"};
  const std::string              ACCEPT_HEADER   = getHeader("Accept");
  std::vector<std::string>       acceptedTypes;

  if (ACCEPT_HEADER.empty() || ACCEPT_HEADER.find("*/*") != std::string::npos) {
    return true;
  } else {
    std::size_t pos = 0;
    std::size_t prev = 0;
    while ((pos = ACCEPT_HEADER.find(',', prev)) != std::string::npos) {
      acceptedTypes.push_back(ACCEPT_HEADER.substr(prev, pos - prev));
      prev = pos + 1;
    }
    acceptedTypes.push_back(ACCEPT_HEADER.substr(prev));
  }

  for (std::vector<std::string>::const_iterator it = acceptedTypes.begin();
       it != acceptedTypes.end(); ++it) {
    for (std::vector<std::string>::const_iterator it2 = TYPES_TO_ACCEPT.begin();
         it2 != TYPES_TO_ACCEPT.end(); ++it2) {
      if (*it == *it2) {
        return true;
      }
    }
  }
  return false;
}

std::string GetRequest::_getErrorPageIndex(const Route &route,
                                           HTTPStatusCode errorCode) const {
  if (route.errorPages.find(errorCode) != route.errorPages.end()) {
    std::string page = route.errorPages.at(errorCode);
    if (_getFileType(page) == FileType::IS_REG_FILE) {
      return route.rootDirectory + page;
    }
  }
  return std::string(DEFAULT_ERROR_PAGE);
}

std::vector<std::string> GetRequest::_getPossiblePaths(
    const std::string &path,
    const std::vector<std::string> &index_files) const {
  std::vector<std::string> possible_paths;

  for (std::vector<std::string>::const_iterator it = index_files.begin();
       it != index_files.end(); ++it) {
    possible_paths.push_back(path + "/" + *it);
  }

  return possible_paths;
}

std::string GetRequest::_constructPath(const std::string &root) const {
  std::string fullPath;

  // eg: root = "root/", _uri = "/index.html"
  // => full_path = "root/index.html"
  if (!root.empty()) {
    return root + _uri;
  } else {
    return _uri;
  }
}

FileType GetRequest::_getFileType(const std::string &path) const {
  Logger &logger = Logger::getInstance();

  struct stat buffer;

  if (stat(path.c_str(), &buffer) == -1) {
    logger.log(
        "[RESPONSE-BUILDING] GetRequest: _fileExists -> File doesn't exist (" +
            path + ")",
        VERBOSE);
    return FileType::IS_UNKNOWN;
  }
  if (buffer.st_mode & S_IFDIR) {
    logger.log(
        "[RESPONSE-BUILDING] GetRequest: _fileExists -> File is a directory (" +
            path + ")",
        VERBOSE);
    return FileType::IS_DIR;
  }
  if (buffer.st_mode & S_IFREG) {
    logger.log(
        "[RESPONSE-BUILDING] GetRequest: _fileExists -> File is a regular file "
        "(" +
            path + ")",
        VERBOSE);
    return FileType::IS_REG_FILE;
  }
  logger.log(
      "[RESPONSE-BUILDING] GetRequest: _fileExists -> File is something else "
      "(" +
          path + ")",
      VERBOSE);
  logger.error("WARNING: logic probably not implemented yet");
  return FileType::IS_UNKNOWN;
}
