#include <RequestParser.hpp>

RequestParser::RequestParser() {}

RequestParser::~RequestParser() {}

static std::string stringTrim(std::string toTrim) {
  size_t i = 0;
  size_t j = toTrim.length();

  while (toTrim[i] == ' ') i++;
  while (toTrim[j] == ' ') j--;
  return toTrim.substr(i, j);
}

/**
 * Start of parsing first line
 */
static int parseMethod(std::string method) {
  if (method == "POST")
    return POST;
  else if (method == "GET")
    return GET;
  else if (method == "DELETE")
    return DELETE;
  return -1;
}

// TODO: check if strings are correct
static int parseHttpVersion(std::string httpVersion) {
  if (httpVersion == "HTTP/1.1")
    return HTTP_1_1;
  else if (httpVersion == "HTTP/2.0")
    return HTTP_2_0;
  else if (httpVersion == "HTTP/3.0")
    return HTTP_3_0;
  return -1;
}

/* Splits string based on "delimiter" */
static std::vector<std::string> splitString(std::string const &str) {
  std::vector<std::string> splittedString;
  std::string currentLine;
  std::string delimiter = " ";

  int start = 0;
  int end = str.find(delimiter);
  while (end != -1) {
    currentLine = str.substr(start, end - start);
    splittedString.push_back(currentLine);
    start = end + delimiter.size();
    end = str.find(delimiter, start);
  }
  currentLine = str.substr(start, end - start);
  splittedString.push_back(currentLine);
  return splittedString;
}

/* All steps involved to parse first line of header */
static int parseFirstLine(HttpHeaderData *dest, std::string const &line) {
  std::vector<std::string> splittedString = splitString(line);
  int retValue;

  if (splittedString.size() != 3) return 1;
  dest->url = splittedString[1];
  retValue = parseMethod(splittedString[0]);
  if (retValue == -1) return 1;
  dest->method = static_cast<EHttpMethods>(retValue);
  retValue = parseHttpVersion(splittedString[2]);
  if (retValue == -1) return 1;
  dest->httpVersion = static_cast<HttpVersion>(retValue);
  return 0;
}

static std::vector<std::string> splitHeader(std::string headerString) {
  std::vector<std::string> headerLines;
  std::string currentLine;
  size_t lastLocation = 0;  // last location of \n\r
  size_t nextLocation = 0;  // next location of \n\r

  do {
    nextLocation = headerString.find("\r\n", nextLocation);
    if (nextLocation == std::string::npos) break;
    currentLine =
        headerString.substr(lastLocation, nextLocation - lastLocation);
    headerLines.push_back(currentLine);
    nextLocation += 2;
    lastLocation = nextLocation;
  } while (true);
  return headerLines;
}

/* Splits header based on \n\r */
static int makeHeaderMap(std::map<std::string, std::string> *pHeadersMap,
                         std::vector<std::string> headerLines) {
  std::string currentLine;
  size_t semiColLocation;
  std::pair<std::string, std::string> pair("", "");

  for (size_t i = 1; i < headerLines.size(); i++) {
    currentLine = headerLines.at(i);
    semiColLocation = currentLine.find(':');
    if (semiColLocation == std::string::npos) return 1;
    pair.first = stringTrim(currentLine.substr(0, semiColLocation));
    pair.second = stringTrim(currentLine.substr(
        semiColLocation + 1, currentLine.length() - semiColLocation));
    pHeadersMap->insert(pair);
  }
  return 0;
}

static HttpRequest *createRequest(HttpHeaderData const &data) {
  Logger &logger = Logger::getInstance();

  switch (data.method) {
    case GET:
      logger.log("Succesfully parsed get request!");
      return new GetRequest(data);
    case POST:
      logger.log("Succesfully parsed post request!");
      return new PostRequest(data);
    case DELETE:
      logger.log("Succesfully parsed delete request!");
      return new DeleteRequest(data);
    default:
      Logger::getInstance().error("Incorrect type of request received");
      return new BadRequest(HTTPStatusCode::BAD_REQUEST);
  }
}

/**
 * End of parsing first line
 */
HttpRequest *RequestParser::parseHeader(std::string &rawRequest) {
  Logger &logger = Logger::getInstance();
  /* Header split into separate strings */
  std::vector<std::string> headerLines;
  /* Data used to initialize a request */
  HttpHeaderData headerData;
  size_t endOfHeader;

  logger.log("Starting to parse request");
  endOfHeader = rawRequest.find("\r\n\r\n");
  if (endOfHeader == std::string::npos) {
    logger.error("Incorrect end of header found -> returning new BadRequest()");
    return new BadRequest(HTTPStatusCode::BAD_REQUEST);
  }
  headerData.body =
      rawRequest.substr(endOfHeader + 4, rawRequest.length() - endOfHeader);
  headerLines = splitHeader(rawRequest.substr(0, endOfHeader + 2));
  if (parseFirstLine(&headerData, headerLines[0])) {
    logger.error(
        "Incorrect first line of request -> returning new BadRequest()");
    return new BadRequest(HTTPStatusCode::BAD_REQUEST);
  }
  if (makeHeaderMap(&headerData.headers, headerLines)) { // headermap always returns 0
    logger.error(
        "Incorrect header added to request -> returning new BadRequest()");
    return new BadRequest(HTTPStatusCode::BAD_REQUEST);
  }
  return createRequest(headerData);
}

HttpRequest *RequestParser::processChunk(std::string &rawRequest)
{
  Logger &logger = Logger::getInstance();
  size_t sub = rawRequest.find("\r\n");
  if (sub == std::string::npos)
  {
    logger.error("[CHUNK]: No \\r\\n pair found");
    return new BadRequest(HTTPStatusCode::BAD_REQUEST);
  }
  std::string sizeStr = rawRequest.substr(sub);

  char  *check = NULL;
  size_t  chunkSize = strtoul(sizeStr.c_str(), &check, 16);
  if (check != NULL)
    return new BadRequest(HTTPStatusCode::BAD_REQUEST);

  HttpHeaderData headerData;

  // last chunk found so there may be extra headers
  if (!chunkSize)
  {
    size_t endOfHeader = rawRequest.find("\r\n\r\n");
    if (endOfHeader == std::string::npos)
    {
      logger.error("[CHUNK]: Incorrect end of header found -> returning new BadRequest()");
      return new BadRequest(HTTPStatusCode::BAD_REQUEST);
    }
    std::vector<std::string> headerLines = splitHeader(rawRequest.substr(0, endOfHeader + 2));
    makeHeaderMap(&headerData.headers, headerLines);
    return new PostRequest(headerData);
  }

  // could add a check for \r\n at the end; if not (single pair) badRequest
  std::string body = unChunk(rawRequest);
  if (body.size() == 0)
    return new BadRequest(HTTPStatusCode::BAD_REQUEST);
  headerData.body = body;
  return new PostRequest(headerData);
}
