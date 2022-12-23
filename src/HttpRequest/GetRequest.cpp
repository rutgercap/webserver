#include <HttpRequest.hpp>

GetRequest::GetRequest(HttpHeaderData const &data) : HttpRequest(data) {}

GetRequest::GetRequest(const GetRequest &ref) : HttpRequest(ref) {}

GetRequest::~GetRequest() {}

HttpResponse GetRequest::_handleCgiRequest(std::string const &path,
                                           Route const &route) const {
  std::map<std::string, std::string> headers;
  std::string body;

  if (!route.cgiEnabled) {
    return _errorResponse(HTTPStatusCode::METHOD_NOT_ALLOWED, path);
  }
  HTTPStatusCode status = CGI::executeFile(&body, &headers, path);

  if (status != HTTPStatusCode::OK) {
    Logger::getInstance().error("Executing cgi: " +
                                getMessageByStatusCode(status));
    return _errorResponse(status, path);
  }
  return _responseWithBody(headers, body);
}

std::string GetRequest::_getErrorPage(const Route &route,
                                      HTTPStatusCode errorCode) const {
  if (route.errorPages.find(errorCode) != route.errorPages.end()) {
    std::string page = route.errorPages.at(errorCode);
    if (_getFileType(page) == FileType::FILE) {
      return route.rootDirectory + page;
    }
  }
  return "";
}

HttpResponse GetRequest::_errorResponse(HTTPStatusCode const &statusCode,
                                        Route const &route) const {
  std::string pathToErrorFile = _getErrorPage(route, statusCode);
  if (pathToErrorFile.empty()) {
    return HttpResponse(HTTPStatusCode::NOT_FOUND);
  }
  return _responseWithFile(pathToErrorFile, statusCode);
}
