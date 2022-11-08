#include "Server.hpp"

#include "Logger.hpp"

/*
 * Constructor for Server class
 */
Server::Server() : _fd(-1), _domain(AF_INET6), _type(SOCK_STREAM), _port(-1), _maxBodySize(-1), _accepted(-1) {
  memset(&_servaddr, 0, sizeof(_servaddr));
  memset(&_buffer, 0, sizeof(_buffer));
  memset(&_defaultErrorPage, 0, sizeof(_defaultErrorPage));
  memset(&_host, 0, sizeof(_host));
  _defaultErrorPage.statusCode = -1;
  _host.statusCode = -1;
}

/*
 * Destructor for Server class
 */
Server::~Server() {
  close(_fd);
}

/*
 * Prepare the socket for incoming connections
 * @param backlog number of connections allowed on the incoming queue
 */
void Server::prepare(const int backlog) {
  Logger &logger = Logger::getInstance();
  int     on     = 1;  // used for setsockopt() and ioctl() calls

  logger.log("Preparing " + _serverName[0] + ":" + std::to_string(_port));

  /**************************************************/
  /* Create an socket to receive incoming           */
  /* connections on                                 */
  /**************************************************/

  _fd = socket(_domain, _type, 0);
  if (_fd == -1) {
    logger.error("Failed to create socket: " + std::string(strerror(errno)));
    throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
  }

  /**************************************************/
  /* Set options on socket to reuse address         */
  /**************************************************/

  if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
    logger.error("Failed to set socket options: " + std::string(strerror(errno)));
    throw std::runtime_error("Socket options failed: " + std::string(strerror(errno)));
  }

  /*************************************************************/
  /* Set socket to be nonblocking. All of the sockets for      */
  /* the incoming connections will also be nonblocking since   */
  /* they will inherit that state from the listening socket.   */
  /*************************************************************/

  if (ioctl(_fd, FIONBIO, (char *)&on) == -1) {
    logger.error("Failed to set socket to nonblocking: " + std::string(strerror(errno)));
    throw std::runtime_error("Socket nonblocking failed: " + std::string(strerror(errno)));
  }

  /**************************************************/
  /* Set all bytes in socket address structure to   */
  /* zero, and fill in the relevant data members    */
  /**************************************************/

  memset(&_servaddr, 0, sizeof(_servaddr));
  _servaddr.sin6_family = _domain;
  memcpy(&_servaddr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
  _servaddr.sin6_port = htons(_port);

  /**************************************************/
  /* Bind the address and port number to the socket */
  /**************************************************/

  if (bind(_fd, (struct sockaddr *)&_servaddr, sizeof(_servaddr)) == -1) {
    logger.error("Failed to bind socket: " + std::string(strerror(errno)));
    throw std::runtime_error("Socket bind failed: " + std::string(strerror(errno)));
  }

  if (listen(_fd, backlog) == -1) {
    logger.error("Failed to prepare socket: " + std::string(strerror(errno)));
    throw std::runtime_error("Socket listen failed: " + std::string(strerror(errno)));
  }
}

/**************************************************/
/* Getters and setters                            */
/**************************************************/

int Server::getFd() const {
  return _fd;
}

std::vector<std::string> Server::getServerName() const {
  return _serverName;
}

PageData Server::getHost() const {
  return _host;
}

PageData Server::getErrorPage() const {
  return _defaultErrorPage;
}

int Server::getPort() const {
  return _port;
}

int Server::getMaxBody() const {
  return _maxBodySize;
}

int Server::setHost(int const &statusCode, std::string const &filePath) {
  if (statusCode < 0) {
    Logger::getInstance().error("Incorrect statuscode set");
    return 1;
  }
  _host.statusCode = statusCode;
  _host.filePath   = filePath;
  return 0;
}

int Server::setErrorPage(int const &statusCode, std::string const &filePath) {
  if (statusCode < 0) {
    Logger::getInstance().error("Incorrect statuscode set");
    return 1;
  }
  _defaultErrorPage.statusCode = statusCode;
  _defaultErrorPage.filePath   = filePath;
  return 0;
}

void Server::setServerName(std::vector<std::string> value) {
  _serverName = value;
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

/**************************************************/
/* End of getters and setters                     */
/**************************************************/

/*
 * To check if variables are set
 */
bool	Server::hasServerName() const {
	return (_serverName.size() == 0);
}

bool	Server::hasHost() const {
	return (_host.statusCode == -1);
}

bool	Server::hasPort() const {
	return (_port != -1);
}

bool	Server::hasMaxBody() const {
	return (_maxBodySize != -1);
}

bool	Server::hasErrorPage() const {
	return (_defaultErrorPage.statusCode == -1);
}

bool	Server::hasRoutes() const {
	return (_routes.size() != 0);
}

/**************************************************/
/* LEGACY CODE                                    */
/*                                                */
/* This code is not used anymore, but is kept     */
/* here for reference                             */
/**************************************************/

/*
 * Accept an incoming connection
 */
void Server::accept_connection() {
  Logger         &logger   = Logger::getInstance();
  const socklen_t sock_len = sizeof(_servaddr);  // size of socket address structure

  {  // This block is just for the logger
    std::ostringstream ss;
    ss << "Waiting for connections on port " << _port;
    logger.log(ss.str());
  }

  /**************************************************/
  /* Accept an incoming connection                  */
  /* - Blocks until a connection is present         */
  /* - Because the accept() function expects a      */
  /* sockaddr structure, it needs to be cast to the */
  /* correct type, the sockaddr_in structure is a   */
  /* "subclass" of sockaddr, so we can cast it to a */
  /* sockaddr structure                             */
  /**************************************************/

  if ((_accepted = accept(_fd, (struct sockaddr *)&_servaddr, (socklen_t *)&sock_len)) == -1) {
    logger.error("Socket accept failed: " + std::string(strerror(errno)));
    throw std::runtime_error("Socket accept failed: " + std::string(strerror(errno)));
  }

  logger.log("New connection accepted");

  /**************************************************/
  /* Read data from the incoming connection         */
  /**************************************************/

  logger.log("Reading data from connection");

  bzero(_buffer, sizeof(_buffer));
  ssize_t bytes_read = read(_accepted, _buffer, sizeof(_buffer));
  if (bytes_read == -1) {
    logger.error("Socket read failed: " + std::string(strerror(errno)));
    throw std::runtime_error("Socket read failed: " + std::string(strerror(errno)));
  }

  if (bytes_read == 0) {
    logger.log("Socket read 0 bytes, ignoring");
    return;
  }

  {  // This block is just for the logger
    std::ostringstream ss;
    ss << "Successfully read " << bytes_read << " bytes from socket";
    logger.log(ss.str());
  }

  /**************************************************/
  /* Log the incoming data                          */
  /**************************************************/

  logger.log("Received data:\n---------------------------\n" + std::string(_buffer) +
             "\n---------------------------\n");

  /**************************************************/
  /* End of data receiver, data is now stored       */
  /* inside the object:                             */
  /* - _buffer: contains the data                   */
  /* - _accepted: contains the accepted socket      */
  /**************************************************/

  return;
}

/*
 * Wait for incoming connections
 */
void Server::wait_for_connections() {
  Logger         &logger   = Logger::getInstance();
  const socklen_t sock_len = sizeof(_servaddr);  // size of socket address structure
  int             new_fd;                        // file descriptor for new socket
  char            buffer[100000] = {0};          // buffer to read incoming data into

  /**************************************************/
  /* Start accepting incoming connections           */
  /**************************************************/

  while (1) {
    {  // This block is just for the logger
      std::ostringstream ss;
      ss << "Waiting for connections on port " << _port;
      logger.log(ss.str());
    }

    /**************************************************/
    /* Accept an incoming connection                  */
    /* - Blocks until a connection is present         */
    /* - Because the accept() function expects a      */
    /* sockaddr structure, it needs to be cast to the */
    /* correct type, the sockaddr_in structure is a   */
    /* "subclass" of sockaddr, so we can cast it to a */
    /* sockaddr structure                             */
    /**************************************************/

    if ((new_fd = accept(_fd, (struct sockaddr *)&_servaddr, (socklen_t *)&sock_len)) == -1) {
      logger.error("Socket accept failed: " + std::string(strerror(errno)));
      throw std::runtime_error("Socket accept failed: " + std::string(strerror(errno)));
    }

    /**************************************************/
    /* Read data from the incoming connection         */
    /**************************************************/

    logger.log("Reading data from socket");

    bzero(buffer, sizeof(buffer));
    ssize_t bytes_read = read(new_fd, buffer, sizeof(buffer));
    if (bytes_read == -1) {
      logger.error("Socket read failed: " + std::string(strerror(errno)));
      throw std::runtime_error("Socket read failed: " + std::string(strerror(errno)));
    }

    if (bytes_read == 0) {
      logger.log("Socket read 0 bytes, ignoring");
      continue;
    }

    {  // This block is just for the logger
      std::ostringstream ss;
      ss << "Successfully read " << bytes_read << " bytes from socket";
      logger.log(ss.str());
    }

    /**************************************************/
    /* Log the received message                       */
    /**************************************************/

    logger.log("Received request\n---------------------------\n" + std::string(buffer) +
               "\n---------------------------\n");

    /**************************************************/
    /* Send response to the client                    */
    /**************************************************/

    {
      HttpRequest  request;
      HttpResponse response;

      request.parse(buffer);
      response.createResponse(request, "root",
                              "index.html");  // TODO: make root configurable, not hardcoded, same for index

      std::string response_str = response.toStr();

      if (write(new_fd, response_str.c_str(), response_str.length()) == -1) {
        logger.error("Socket write failed: " + std::string(strerror(errno)));
        throw std::runtime_error("Socket write failed: " + std::string(strerror(errno)));
      }
    }

    /**************************************************/
    /* Close the connection                           */
    /**************************************************/

    if (close(new_fd) == -1) {
      logger.error("Socket close failed: " + std::string(strerror(errno)));
      throw std::runtime_error("Socket close failed: " + std::string(strerror(errno)));
    }
  }
}

/**************************************************/
/* END OF LEGACY CODE                             */
/**************************************************/