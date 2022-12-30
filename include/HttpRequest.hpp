#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <CGI.hpp>
#include <RequestParser.hpp>
#include <Server.hpp>
#include <StatusCodes.hpp>
#include <Webserver.hpp>
#include <string>
#include <vector>
#include <dirent.h>

#include "HttpMessage.hpp"
#include "HttpResponse.hpp"

class Server;
class HttpResponse;
struct Route;
struct HttpHeaderData;

// HTTP REQUEST BASE
class HttpRequest : public HttpMessage {
 public:
  HttpRequest();
  HttpRequest(HttpHeaderData const &data);
  HttpRequest(const HttpRequest &obj);
  virtual ~HttpRequest();

  // Abstract
  virtual HTTPStatusCode executeRequest(const Server &server)    = 0;
  virtual HttpResponse   constructResponse(const Server &server) = 0;

 protected:
  EHttpMethods _method;
  std::string  _uri;
  bool         _chunked;

  // Protected methods
  bool _isMethodAllowed(const std::map<EHttpMethods, bool> allowedMethods) const;
};

// GET

enum class FileType { IS_DIR, IS_REG_FILE, IS_UNKNOWN };

// # define IS_DIR      0
// # define IS_REG_FILE 1
// # define IS_UNKNOWN  2

class GetRequest : public HttpRequest {
 public:
  GetRequest(HttpHeaderData const &data);
  GetRequest(const GetRequest &ref);
  ~GetRequest();

  // Concrete
  HTTPStatusCode executeRequest(const Server &server);
  HttpResponse   constructResponse(const Server &server);

 private:
  // Private methods
  HttpResponse             _responseWithAutoIndex(const std::string &path) const;
  bool                     _typeIsAccepted() const;
  std::string              _getErrorPageIndex(const Route &route, HTTPStatusCode errorCode) const;
  std::vector<std::string> _getPossiblePaths(const std::string &path, const std::vector<std::string> &index_files);
  std::vector<std::string> _getAcceptedTypesFromHeader() const;
  std::string              _constructPath(const std::string &root) const;
  FileType                 _fileExists(const std::string &path) const;
  HttpResponse _createResponseObject(const std::string &path, HTTPStatusCode statusCode, const Route &route) const;
  bool         _checkIfIndexFileExists(const std::vector<std::string> &indexFiles, const std::string &path) const;
  std::vector<std::string> _getPossiblePaths(const std::string              &path,
                                             const std::vector<std::string> &index_files) const;
  FileType                 _getFileType(const std::string &path) const;
  HttpResponse             _handleCgiRequest(std::string const &path, Route const &route) const;
  HttpResponse             _handleFileRequest(std::string const &path, Route const &route) const;
  HttpResponse             _errorResponseWithFile(HTTPStatusCode const &statusCode, Route const &route) const;
  HttpResponse             _responseWithFile(std::string const &path, HTTPStatusCode statusCode) const;
  HttpResponse _responseWithBody(const std::map<std::string, std::string> &headers, const std::string &body) const;
};

// DELETE

class DeleteRequest : public HttpRequest {
 public:
  DeleteRequest(HttpHeaderData const &data);
  DeleteRequest(const DeleteRequest &ref);
  ~DeleteRequest();

  // Concrete
  HTTPStatusCode executeRequest(const Server &server);
  HttpResponse   constructResponse(const Server &server);
};

// POST

class PostRequest : public HttpRequest {
 public:
  PostRequest(HttpHeaderData const &data);
  PostRequest(const PostRequest &ref);
  ~PostRequest();

  // Concrete
  HTTPStatusCode executeRequest(const Server &server);
  HttpResponse   constructResponse(const Server &server);
};

// BAD

class BadRequest : public HttpRequest {
 public:
  BadRequest(HTTPStatusCode statusCode);
  BadRequest(const BadRequest &ref);
  ~BadRequest();

  // Concrete
  HTTPStatusCode executeRequest(const Server &server);
  HttpResponse   constructResponse(const Server &server);

 private:
  HTTPStatusCode _statusCode;
};

#endif  // HTTP_REQUEST_HPP
