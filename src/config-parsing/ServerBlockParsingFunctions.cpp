#include <Parser.hpp>

/**************************************************/
/* Parsing abstract syntax tree lines	          */
/**************************************************/

t_parseFuncPair Parser::lineParsingFuncs[PARSER_FUNC_N] = {
    {"listen", &Parser::parsePort},
    {"serverName", &Parser::parseServerName},
    {"maxBodySize", &Parser::parseMaxBodySize},
};

int Parser::parseServerName(void *dest, t_dataLine line) {
  if (!dest || line.size() < 2) {
    return 1;
  }
  Server                            *server     = static_cast<Server *>(dest);
  std::string                        serverName = "";
  std::vector<std::string>::iterator it         = line.begin();
  it++;
  while (it != line.end()) {
    if (serverName.length() != 0) {
      serverName += " ";
    }
    serverName += *it;
    it++;
  }
  server->setServerName(serverName);
  return 0;
}

static bool isAllNumbers(std::string const &line) {
  for (size_t i = 0; i < line.length(); i++) {
    if (!isdigit(line[i])) {
      return false;
    }
  }
  return true;
}

int Parser::parsePort(void *dest, t_dataLine line) {
  if (!dest || line.size() != 2) {
    return 1;
  }
  Server     *server  = static_cast<Server *>(dest);
  std::string rawLine = line.at(1);
  size_t      port    = 0;
  /* In case only port is entered */
  if (!isAllNumbers(rawLine)) {
    return 1;
  }
  port = std::stoi(rawLine);
  return server->setPort(port);
}

int Parser::parseMaxBodySize(void *dest, t_dataLine line) {
  if (!dest || line.size() != 2) {
    return 1;
  }
  Server     *server = static_cast<Server *>(dest);
  std::string maxBodySize;

  maxBodySize = line.at(1);
  for (size_t i = 0; i < maxBodySize.length(); i++) {
    if (!isdigit(maxBodySize[i])) {
      return 1;
    }
  }
  return server->setMaxBody(std::stoi(line.at(1)));
}
