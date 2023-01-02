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
  HTTPStatusCode status =
      CGI::executeFile(&body, &headers, route.cgiParams, path);

  if (status != HTTPStatusCode::OK) {
    Logger::getInstance().error("Executing cgi: " +
                                getMessageByStatusCode(status));
    return _errorResponse(status, path);
  }
  return _responseWithBody(headers, body);
}

#define DEFAULT_ERROR_PAGE "root/error_pages/404/index.html"
std::string GetRequest::_getErrorPage(const Route &route,
                                      HTTPStatusCode errorCode) const {
  if (route.errorPages.find(errorCode) != route.errorPages.end()) {
    std::string page =
        route.rootDirectory + "/" + route.errorPages.at(errorCode);
    if (_getFileType(page) == FileType::FILE) {
      return page;
    }
  }
  return std::string(DEFAULT_ERROR_PAGE);
}

HttpResponse GetRequest::_errorResponse(HTTPStatusCode const &statusCode,
                                        Route const &route) const {
  std::string pathToErrorFile = _getErrorPage(route, statusCode);
  return _responseWithFile(pathToErrorFile, statusCode);
}
