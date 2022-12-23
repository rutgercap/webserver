#include "HttpResponse.hpp"

#include "Logger.hpp"

HttpResponse::HttpResponse(HTTPStatusCode status)
    : HttpMessage(HttpVersion::HTTP_1_1),
      _statusCode(status),
      _statusMessage(getMessageByStatusCode(status)) {}

HttpResponse::HttpResponse(HTTPStatusCode status,
                           std::map<std::string, std::string> const &headers,
                           std::string const &body)
    : HttpMessage(HttpVersion::HTTP_1_1, headers, body),
      _statusCode(status),
      _statusMessage(getMessageByStatusCode(status)) {}

HttpResponse::HttpResponse() : HttpMessage(HttpVersion::HTTP_1_1) {}

HttpResponse::HttpResponse(const HttpResponse &obj) : HttpMessage(obj) {
  _statusCode = obj._statusCode;
  _statusMessage = obj._statusMessage;
}

HttpResponse::~HttpResponse() {}

/*
 * Convert the response to a string
 * @return The response as a string
 */
std::string HttpResponse::toStr() const {
  Logger &logger = Logger::getInstance();

  logger.log("< Started creating response string", VERBOSE);
  std::string responseString;

  {
    logger.log("1. Adding status line", VERBOSE);
    std::ostringstream ss;
    ss << getVersionToStr() << " " << static_cast<int>(_statusCode) << " "
       << _statusMessage << "\r\n";
    responseString += ss.str();
  }

  logger.log("2. Adding headers", VERBOSE);
  {
    std::map<std::string, std::string>::const_iterator it;

    for (it = _headers.begin(); it != _headers.end(); it++) {
      responseString += it->first + ": " + it->second + "\r\n";
    }

    responseString +=
        "\r\n";  // Add an extra CRLF to separate headers from body
  }

  logger.log("3. Adding body", VERBOSE);
  if (_body.length() > 0) {
    responseString += _body;
  }
  return responseString;
}
