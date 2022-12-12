#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <General.hpp>

#include <HttpRequest.hpp>
#include <HttpResponse.hpp>
#include <Logger.hpp>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <climits>

#define MAX_PORT 65535

/* Default values */
#define DEFAULT_ERROR_STATUS 404
#define DEFAULT_ERROR_PATH "error.html"

#define DEFAULT_HOST_STATUS 420
#define DEFAULT_HOST_PATH "index.html"

#define DEFAULT_MAX_BODY 1000000

#define DEFAULT_IP_ADRESS "0.0.0.0"
#define DEFAULT_PORT 80

#define ROOT_FOLDER "root/"
/* End of default values */

struct Route
{
  Route(std::string const &name) : route(name), rootDirectory(ROOT_FOLDER), defaultFile("index.html"), autoIndex(false)
  {
    allowedMethods[GET] = true;
    allowedMethods[POST] = true;
    allowedMethods[DELETE] = true;
  }
  // Default route constructor
  Route() : route("/"), rootDirectory(ROOT_FOLDER), cgiRoot(ROOT_FOLDER), defaultFile("index.html"), httpRedirection("")
  {
    allowedMethods[GET] = true;
    allowedMethods[POST] = true;
    allowedMethods[DELETE] = true;
  }
  // Vector for bonus
  std::string route;
  std::string rootDirectory;
  std::string cgiRoot;
  std::string defaultFile;
  std::map<EHttpMethods, bool> allowedMethods;
  std::vector<std::string> indexFiles;
  std::string httpRedirection;
  bool autoIndex;
};

struct PageData
{
  PageData(int const &statusCode, std::string const &filePath) : statusCode(statusCode), filePath(filePath) {}

  int statusCode;
  std::string filePath;
};

class Server
{
public:
  Server();
  ~Server();

  //  Getters
  std::vector<std::string> getServerName() const;
  std::vector<Route> getRoutes() const;
  PageData getHost() const;
  PageData getErrorPage() const;
  int getMaxBody() const;
  int getPort() const;
  int getFd() const;
  std::vector<int> &getConnectedClients();
  HttpRequest *getRequestByDiscriptor(int fd);

  // Setters
  int setHost(int const &statusCode, std::string const &filePath);
  int setErrorPage(int const &statusCode, std::string const &filePath);
  void setServerName(std::vector<std::string> const &value);
  int setPort(int const &value);
  int setMaxBody(double const &value);
  void addRoute(Route const &route);

  // Request generation
  HttpRequest *createRequest(std::string &msg);

private:
  int _port;
  int _maxBodySize;
  std::vector<std::string> _serverName;
  std::vector<Route> _routes;
  PageData _defaultErrorPage;
  PageData _host;
  std::string _ipAddress;
  std::vector<HttpRequest *> _unhandledRequests;
};
