#include "Socket.hpp"

/*
 * Constructor for Socket class
 * @param domain communication domain
 * @param type communication semantics
 * @param protocol protocol to be used with the socket
 * @param port port number of socket
 */
Socket::Socket(const int domain, const int type, const int protocol, const int port) : _port(port) {
  /**************************************************/
  /* Create an socket to receive incoming           */
  /* connections on                                 */
  /**************************************************/

  _fd = socket(domain, type, protocol);
  if (_fd == -1) {
    throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
  }

  // TEMPORARY SOLUTION TO ENABLE REUSING OF PORTS
  int opt = 1;
  setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  /**************************************************/
  /* Set all bytes in socket address structure to   */
  /* zero, and fill in the relevant data members    */
  /**************************************************/

  memset(&_servaddr, 0, sizeof(_servaddr));
  _servaddr.sin_family      = domain;
  _servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  _servaddr.sin_port        = htons(_port);

  /**************************************************/
  /* Bind the address and port number to the socket */
  /**************************************************/

  if (bind(_fd, (struct sockaddr *)&_servaddr, sizeof(_servaddr)) == -1) {
    throw std::runtime_error("Socket bind failed: " + std::string(strerror(errno)));
  }
}

/*
 * Prepare the socket for incoming connections
 * @param backlog number of connections allowed on the incoming queue
 */
void Socket::prepare(const int backlog) const {
  if (listen(_fd, backlog) == -1) {
    throw std::runtime_error("Socket listen failed: " + std::string(strerror(errno)));
  }
}

/*
 * Wait for incoming connections
 */
void Socket::wait_for_connections() {
  const socklen_t sock_len = sizeof(_servaddr);  // size of socket address structure
  int             new_fd;                        // file descriptor for new socket
  char            buffer[50000] = {0};           // buffer to read incoming data into

  /**************************************************/
  /* Start accepting incoming connections           */
  /**************************************************/

  while (1) {
    std::cout << "Waiting for connections on port " << _port << "..." << std::endl;

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
      throw std::runtime_error("Socket accept failed: " + std::string(strerror(errno)));
    }

    /**************************************************/
    /* Read data from the incoming connection         */
    /**************************************************/

    bzero(buffer, sizeof(buffer));
    if (read(new_fd, buffer, sizeof(buffer)) == -1) {
      throw std::runtime_error("Socket read failed: " + std::string(strerror(errno)));
    }

    /**************************************************/
    /* Print the received message                     */
    /**************************************************/

    std::cout << "Received message from " << inet_ntoa(_servaddr.sin_addr) << ":" << ntohs(_servaddr.sin_port)
              << std::endl;
    std::cout << buffer << std::endl;

    /**************************************************/
    /* Send response to the client                    */
    /**************************************************/

    HttpRequest request;
    request.parse(buffer);
    std::string response = get_response_to_str(request);  // Get the response to the request
    if (write(new_fd, response.c_str(), response.length()) == -1) {
      throw std::runtime_error("Socket write failed: " + std::string(strerror(errno)));
    }

    /**************************************************/
    /* Close the connection                           */
    /**************************************************/

    if (close(new_fd) == -1) {
      throw std::runtime_error("Socket close failed: " + std::string(strerror(errno)));
    }
  }
}

/*
 * Destructor for Socket class
 */
Socket::~Socket() {
  /**************************************************/
  /* Close the socket                               */
  /**************************************************/

  if (close(_fd) == -1) {
    throw std::runtime_error("Socket close failed: " + std::string(strerror(errno)));
  }
}

/*
 * Get response to the request
 * @param request request to get response to
 * @return response to the request
 */
std::string Socket::get_response_to_str(const HttpRequest &request) const {
  (void)request;

  // By default send back 404 Not Found which is located at /root/404/404.html
  std::string header = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n";

  // Find Content-Length which is the size of the file (/root/404/404.html)
  std::ifstream file("root/404/404.html", std::ios::binary);
  if (file.is_open()) {
    file.seekg(0, std::ios::end);
    header += "Content-Length: " + std::to_string(file.tellg()) + "\r\n\r\n";
    file.close();
  } else {
    throw std::runtime_error("File not found");
  }

  // Fill the body of the response with the content of the file
  std::ifstream file2("root/404/404.html", std::ios::binary);
  if (file2.is_open()) {
    std::string body((std::istreambuf_iterator<char>(file2)), std::istreambuf_iterator<char>());
    file2.close();
    header += body;
  } else {
    throw std::runtime_error("File not found");
  }

  return header;
}
