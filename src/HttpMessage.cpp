#include "HttpMessage.hpp"

HttpMessage::HttpMessage() {}

HttpMessage::HttpMessage(std::map<std::string, std::string> const &headers, HttpVersion const &version, std::string const &body) : _version(version), _headers(headers), _body(body) {}

HttpMessage::HttpMessage(std::map<std::string, std::string> const &headers, HttpVersion const &version) : _version(version), _headers(headers) {}

HttpMessage::HttpMessage(HttpVersion const &version) : _version(version) {}

HttpMessage::~HttpMessage() {}

/**************************************************/
/* Start of the getters                           */
/**************************************************/

/*
 * Getter for the HTTP version of the message
 */
HttpVersion HttpMessage::getVersion() const
{
  return _version;
}

/*
 * Getter for the HTTP version of the message as a string
 */
std::string HttpMessage::getVersionToStr() const
{
  switch (_version)
  {
  case HTTP_1_1:
    return "HTTP/1.1";
  case HTTP_2_0:
    return "HTTP/2.0";
  case HTTP_3_0:
    return "HTTP/3.0";
  default:
    return "HTTP/1.1";
  }
}

/*
 * Getter for the headers of the message
 */
std::map<std::string, std::string> HttpMessage::getHeaders() const
{
  return _headers;
}

/*
 * Getter for a specific header of the message
 * @param key the key of the header
 * @return the value of the header
 */
std::string HttpMessage::getHeader(const std::string &key) const
{
  return _headers.at(key);
}

/*
 * Getter for the body of the message
 */
std::string HttpMessage::getBody() const
{
  return _body;
}

/**************************************************/
/* End of the getters                             */
/**************************************************/

/*
 * Parse the version string into the version enum
 * @param version the version string
 * @return the version enum
 */
HttpVersion HttpMessage::_parseVersion(const std::string &version)
{
  std::map<std::string, HttpVersion> version_map;

  version_map["HTTP/1.1"] = HTTP_1_1;
  version_map["HTTP/2.0"] = HTTP_2_0;
  version_map["HTTP/3.0"] = HTTP_3_0;

  return version_map.at(version);
}
