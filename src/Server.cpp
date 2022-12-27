#include "Server.hpp"

#include "Logger.hpp"

/*
 * Constructor for Server class
 */
Server::Server()
    : _serverName(DEFAULT_HOST),
      _defaultErrorPage(PageData(DEFAULT_ERROR_STATUS, DEFAULT_ERROR_PATH)) {}

/*
 * Destructor for Server class
 */
Server::~Server() {}

/**************************************************/
/* Getters                                        */
/**************************************************/

std::string Server::getServerName() const { return _serverName; }

PageData Server::getErrorPage() const { return _defaultErrorPage; }

int Server::getPort() const { return _port; }

std::vector<Route> Server::getRoutes() const { return _routes; }

static int matchValue(const std::string &option, const std::string &uri) {
  std::string urilong = uri.back() == '/' ? uri : uri + "/";
  while (urilong.size()) {
    if (option.size() > urilong.size()) return 0;
    if (urilong == option) return (option.size());
    size_t find = urilong.rfind('/', urilong.size() - 2);
    if (find == std::string::npos) return (0);
    urilong = urilong.substr(0, find + 1);
  }
  return 0;
}

// Possible TODO: look for longest matching route
const Route &Server::getRoute(const std::string &uri) const {
  const Route *match = &_routes[0];
  int value = 0;
  for (std::vector<Route>::const_iterator it = _routes.begin();
       it != _routes.end(); it++) {
    const Route &route = *it;
    if (matchValue(route.route, uri) > value) {
      match = &route;
      value = route.route.size();
    }
  }
  return *match;
}

void Server::setServerName(std::string const &value) { _serverName = value; }

int Server::setErrorPage(int const &statusCode, std::string const &filePath) {
  if (statusCode < 0) {
    Logger::getInstance().error("Incorrect statuscode set");
    return 1;
  }
  _defaultErrorPage.statusCode = statusCode;
  _defaultErrorPage.filePath = filePath;
  return 0;
}

int Server::setPort(int const &value) {
  if (value > MAX_PORT) {
    Logger::getInstance().error("Port higher than MAX_PORT");
    return 1;
  } else if (value <= 0) {
    Logger::getInstance().error("Port higher than MAX_PORT");
    return 1;
  }
  _port = value;
  return 0;
}

void Server::addRoute(Route const &route) { _routes.push_back(route); }

std::ostream &operator<<(std::ostream &os, const Server &ob) {
  os << ob.getServerName() << "\n\t" << ob.getErrorPage().filePath << "\n\t"
     << ob.getRoute("/").route;
  return os;
}
