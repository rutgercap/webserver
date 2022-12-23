#ifndef SERVER_HPP
#define SERVER_HPP

#include <unistd.h>

#include <CGI.hpp>
#include <HttpRequest.hpp>
#include <HttpResponse.hpp>
#include <Logger.hpp>
#include <Webserver.hpp>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define MAX_PORT 65535

/* Default values */
#define DEFAULT_ERROR_STATUS 404
#define DEFAULT_ERROR_PATH "error.html"

#define DEFAULT_HOST_STATUS 420
#define DEFAULT_HOST_PATH "index.html"

#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_PORT 80

/* End of default values */

struct Route {
  Route(std::string const &name)
      : route(name),
        defaultFile("index.html"),
        autoIndex(false),
        cgiEnabled(false),
        maxBodySize(0) {
    allowedMethods[GET] = true;
    allowedMethods[POST] = true;
    allowedMethods[DELETE] = true;
  }
  // Default route constructor
  Route()
      : route("/"),
        defaultFile("index.html"),
        httpRedirection(""),
        cgiEnabled(false),
        maxBodySize(0) {
    allowedMethods[GET] = true;
    allowedMethods[POST] = true;
    allowedMethods[DELETE] = true;
  }
  // Vector for bonus
  std::string route;
  std::string rootDirectory;
  std::string defaultFile;
  std::map<HTTPStatusCode, std::string> errorPages;
  std::map<EHttpMethods, bool> allowedMethods;
  std::vector<std::string> indexFiles;
  std::string httpRedirection;
  bool autoIndex;
  bool cgiEnabled;
  size_t maxBodySize;
};

struct PageData {
  PageData(int const &statusCode, std::string const &filePath)
      : statusCode(statusCode), filePath(filePath) {}
  int statusCode;
  std::string filePath;
};

class Server {
 public:
  Server();
  ~Server();

  std::string getServerName() const;
  std::vector<Route> getRoutes() const;
  PageData getErrorPage() const;
  int getPort() const;
  int setPort(int const &value);
  int setErrorPage(int const &statusCode, std::string const &filePath);
  void setServerName(std::string const &value);
  void addRoute(Route const &route);
  const Route &getRoute(const std::string &uri) const;

 private:
  int _port;
  std::string _serverName;
  std::vector<Route> _routes;
  PageData _defaultErrorPage;
};

std::ostream &operator<<(std::ostream &os, const Server &ob);

#endif