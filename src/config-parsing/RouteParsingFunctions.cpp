#include <Parser.hpp>
#include <exception>

/**************************************************/
/* Parsing abstract syntax tree blocks	          */
/**************************************************/

t_parseFuncPair Parser::blockParsingFuncs[BLOCK_FUNC_N] = {
    {"root", &Parser::parseRoot},
    {"index", &Parser::parseIndex},
    {"autoIndex", &Parser::parseAutoIndex},
    {"methods", &Parser::parseMethods},
    {"errorPage", &Parser::parseErrorPage},
    {"cgi", &Parser::parseCgi},
    {"return", &Parser::parseRedirection},
    {"cgi_params", &Parser::parseCgiParam},
};

int Parser::parseCgiParam(void *dest, t_dataLine line) {
  if (!dest || line.size() < 2) {
    Logger::getInstance().error("[CONFIG PARSER]: Invalid index directive");
    return 1;
  }
  Route *route = static_cast<Route *>(dest);

  for (std::size_t i = 1; i < line.size(); i++) {
    route->cgiParams.push_back(line.at(i));
  }
  return 0;
}

int Parser::parseRedirection(void *dest, t_dataLine line) {
  if (!dest || line.size() != 3) {
    return 1;
  }
  int redirCode = stoi(line[1]);
  if (redirCode < 300 || redirCode >= 400) {
    return 1;
  }
  Route *route = static_cast<Route *>(dest);
  std::pair<int, std::string> *result = &route->redirection;
  result->first = redirCode;
  result->second = line[2];
  return 0;
}

int Parser::parseErrorPage(void *dest, t_dataLine line) {
  if (!dest || line.size() != 3) {
    return 1;
  }

  Route *route = static_cast<Route *>(dest);

  try {
    int errorCode = std::stoi(line.at(1));
    if (errorCode < 400 || errorCode > 599) {
      Logger::getInstance().error(
          "[CONFIG PARSER]: Invalid error code for error page");
      return 1;
    }
    HTTPStatusCode code = static_cast<HTTPStatusCode>(errorCode);
    route->errorPages[code] = line.at(2);
  } catch (std::exception &e) {
    Logger::getInstance().error("[CONFIG PARSER]: errorPage is not a number");
    return 1;
  }

  return 0;
}

int Parser::parseMethods(void *dest, t_dataLine line) {
  if (!dest || line.size() < 2 || line.size() > 4) {
    return 1;
  }
  Route *route = static_cast<Route *>(dest);
  route->allowedMethods[GET] = false;
  route->allowedMethods[POST] = false;
  route->allowedMethods[DELETE] = false;
  for (size_t i = 1; i < line.size(); i++) {
    if (line.at(i) == "GET") {
      if (route->allowedMethods[GET] == true) {
        return 1;
      }
      route->allowedMethods[GET] = true;
    } else if (line.at(i) == "POST") {
      if (route->allowedMethods[POST] == true) {
        return 1;
      }
      route->allowedMethods[POST] = true;
    } else if (line.at(i) == "DELETE") {
      if (route->allowedMethods[DELETE] == true) {
        return 1;
      }
    }
  }
  return 0;
}

int Parser::parseRoot(void *dest, t_dataLine line) {
  if (!dest || line.size() != 2) {
    return 1;
  }
  Route *route = static_cast<Route *>(dest);

  if (route->rootDirectory.length() != 0) {
    Logger::getInstance().error(
        "[CONFIG PARSER]: Root directory for route already set");
    return 1;
  }
  route->rootDirectory = line.at(1);
  return 0;
}

int Parser::parseIndex(void *dest, t_dataLine line) {
  if (!dest || line.size() < 2) {
    Logger::getInstance().error("[CONFIG PARSER]: Invalid index directive");
    return 1;
  }

  Route *route = static_cast<Route *>(dest);

  for (std::size_t i = 1; i < line.size(); i++) {
    if (std::find(route->indexFiles.begin(), route->indexFiles.end(),
                  line.at(i)) == route->indexFiles.end()) {
      route->indexFiles.push_back(line.at(i));
    }
  }

  return 0;
}

int Parser::parseCgi(void *dest, t_dataLine line) {
  if (!dest || line.size() != 2) {
    Logger::getInstance().error("Cgi route invalid");
    return 1;
  }
  Route *route = static_cast<Route *>(dest);
  std::string enabled = line.at(1);
  if (enabled == "true") {
    route->cgiEnabled = true;
  } else if (enabled == "false") {
    route->cgiEnabled = false;
  } else {
    Logger::getInstance().error("Invalid cgi param given");
    return 1;
  }
  return 0;
}

int Parser::parseAutoIndex(void *dest, t_dataLine line) {
  if (!dest || line.size() != 2) {
    Logger::getInstance().error("Auto index invalid");
    return 1;
  }
  Route *route = static_cast<Route *>(dest);
  std::string enabled = line.at(1);
  if (enabled == "on") {
    route->autoIndex = true;
  } else if (enabled == "off") {
    route->autoIndex = false;
  } else {
    Logger::getInstance().error("Auto index param given");
    return 1;
  }
  return 0;
}
