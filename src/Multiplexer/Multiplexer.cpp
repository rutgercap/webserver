#include "Multiplexer.hpp"

#include "Logger.hpp"
#include <algorithm>

Multiplexer::Multiplexer(std::vector<Socket> sockets) : _sockets(sockets), _endServer(false)
{
    _addSocketsAsPollFd(POLLIN);
}

Multiplexer::~Multiplexer() {}

void Multiplexer::_addSocketsAsPollFd(short const &events)
{
    Logger &logger = Logger::getInstance();

    for (std::vector<Socket>::iterator it = _sockets.begin(); it != _sockets.end(); it++)
    {
        pollfd serverAsPollFD = {it->getFd(), events, 0};
        _clients.push_back(serverAsPollFD);
    }
    logger.log("[PREPARING] Multiplexer: sockets added to multiplexer, total number of stored sockets: " +
               std::to_string(_clients.size()));
}

int Multiplexer::evaluateClient(pollfd *client)
{
    Logger &logger = Logger::getInstance();
    const int eventType = _getEvent(*client);
    const int clientFd = client->fd;

    switch (eventType)
    {
    case POLLIN:
    {
        if (_isSocket(clientFd))
            _addClient(clientFd); // TODO: Oswin: fix this shit
        else
        {
            int responseStatus = _processRequest(clientFd);
            if (responseStatus == 2)
                return clientFd;
            // TODO: is this correct here or after POLLOUT?
            else if (responseStatus < 0)
                return -1;
            client->revents = POLLOUT;
        }
        break;
    }

    case POLLOUT:
    {
        // std::string tmp_index("index.html");
        // Socket &clientSocket = _getSocketForClient(clientFd);
        // HttpRequest *client_request = clientSocket.getRequestByDiscriptor(clientFd);
        // if (client_request)
        // {
        //   HttpResponse client_response =
        //       client_request->constructResponse(clientSocket, tmp_index); // index.html shouldnt be hardcoded..
        //   send(clientFd, (void *)client_response.toStr().c_str(), client_response.toStr().size(), 0);
        // }
        // delete client_request;
        // markForRemoval.push_back(clientFd);
        // break;
    }

    // case POLLOUT, POLLERR, POLLHUP?
    case POLLNVAL:
    {
        logger.error("POLLNVAL on descriptor : " + std::to_string(clientFd));
        break;
    }
    default:
    {
        logger.log("[POLLING] Multiplexer: No events on socket");
        break;
    }
    }
    return 0;
}

/*
 * Wait for events on the clients
 * @param timeout Timeout in milliseconds, -1 for infinite
 */
void Multiplexer::waitForEvents(const int timeout)
{
    Logger &logger = Logger::getInstance();
    std::vector<int> markForRemoval;

    logger.log("[POLLING] Multiplexer: Starting poll() loop with timeout of " + std::to_string(timeout) +
               " milliseconds");
    do
    {
        if (_pollSockets(timeout) == -1)
            break;
        logger.log("[POLLING] Multiplexer: Poll returned successfully, checking for events on sockets");

        /* Loop over clients and evaluate */
        for (size_t i = 0; i < _clients.size(); i++)
        {
            int toRemove = evaluateClient(&_clients[i]);
            if (toRemove > 0)
            {
                markForRemoval.push_back(toRemove);
            }
        }

        /* Remove any handled clients */
        for (size_t i = 0; i < markForRemoval.size(); i++)
            _removeClient(markForRemoval[i]);

        markForRemoval.clear();
    } while (_endServer == false);
}

/*
 * TODO: move this to socket class ?
 * Send data to a client
 * @param clientSocket The socket of the client
 * @param data The data to send
 * @return The number of bytes send
 */
int Multiplexer::_sendData(const int socket, const std::string &data) const
{
    Logger &logger = Logger::getInstance();

    logger.log("[WRITING] Multiplexer: Writing data to client: \n" + data);
    return write(socket, data.c_str(), data.length());
}

/*
 * Check if a fd is a server
 * fd can be either a client or server
 */
bool Multiplexer::_isSocket(const int fd) const
{
    Logger &logger = Logger::getInstance();

    for (std::vector<Socket>::const_iterator it = _sockets.begin(); it != _sockets.end(); ++it)
    {
        if (it->getFd() == fd)
        {
            logger.log("[POLLING] Multiplexer: Socket is a server [" + std::to_string(it->getFd()) + ":" +
                       std::to_string(it->getPort()) + "]");
            return true;
        }
    }
    return false;
}

Socket Multiplexer::_getSocketByFd(const int fd) const
{
    for (std::vector<Socket>::const_iterator it = _sockets.begin(); it != _sockets.end(); ++it)
    {
        if (it->getFd() == fd)
            return *it;
    }
    // TODO: fix error handling?
    return Socket(-1);
}

void Multiplexer::_addClient(const int socket)
{
    Logger &logger = Logger::getInstance();

    logger.log("[POLLING] Multiplexer: Adding server socket " + std::to_string(socket) + " to multiplexer");
    int newSocket = accept(socket, nullptr, nullptr);
    if (newSocket == -1)
    {
        logger.error("[POLLING] Multiplexer: Failed to accept new connection: " + std::string(strerror(errno)));
        _endServer = true;
        return;
    }
    logger.log("[POLLING] Multiplexer: New connection accepted: " + std::to_string(newSocket));
    pollfd client = {newSocket, POLLIN | POLLOUT, 0};
    _clients.push_back(client);
    for (std::vector<Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it)
    {
        if (it->getFd() == socket)
        {
            it->addClient(newSocket);
            break;
        }
    }
    logger.log("[POLLING] Multiplexer: New connection added to multiplexer, total number of stored sockets: " +
               std::to_string(_clients.size()));
}

/*
 * Remove a client socket from the multiplexer
 * @param socket The socket to remove from the multiplexer
 */
void Multiplexer::_removeClient(const int socket)
{
    Logger &logger = Logger::getInstance();

    logger.log("[POLLING] Multiplexer: Removing client socket " + std::to_string(socket) + " from multiplexer");
    for (std::vector<pollfd>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->fd == socket)
        {
            close(socket);
            _clients.erase(it);
            logger.log("[POLLING] Multiplexer: Client socket " + std::to_string(socket) + " removed from multiplexer");
            break;
        }
    }
    for (std::vector<Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it)
    {
        it->removeClient(socket);
    }
}

/*
 * Check for an return event on a socket
 * @param fd The pollfd to check
 */
int Multiplexer::_getEvent(const pollfd &fd)
{
    Logger &logger = Logger::getInstance();

    logger.log("[POLLING] Multiplexer: Checking for event on socket " + std::to_string(fd.fd));
    if (fd.revents == 0)
    {
        logger.log("[POLLING] Multiplexer: No event on socket " + std::to_string(fd.fd));
        return 0;
    }
    else if (fd.revents & POLLIN)
    {
        logger.log("[POLLING] Multiplexer: POLLIN event on socket " + std::to_string(fd.fd));
        return POLLIN;
    }
    else if (fd.revents & POLLOUT)
    {
        logger.log("[POLLING] Multiplexer: POLLOUT event on socket " + std::to_string(fd.fd));
        return POLLOUT;
    }
    else if (fd.revents & POLLERR)
    {
        logger.log("[POLLING] Multiplexer: POLLERR event on socket " + std::to_string(fd.fd));
        return POLLERR;
    }
    else if (fd.revents & POLLHUP)
    {
        logger.log("[POLLING] Multiplexer: POLLHUP event on socket " + std::to_string(fd.fd));
        return POLLHUP;
    }
    else
    {
        logger.log("[POLLING] Multiplexer: Unknown event on socket " + std::to_string(fd.fd));
        return -1;
    }
}

/*
 * Poll the sockets
 * @param timeout Timeout in milliseconds, -1 for infinite
 * @return 0 on success, -1 on error
 */
int Multiplexer::_pollSockets(const int timeout)
{
    Logger &logger = Logger::getInstance();
    pollfd *fds = &_clients[0];

    logger.log("[POLLING] Multiplexer: Polling for events on " + std::to_string(_clients.size()) + " sockets...");
    int pollResult = poll(fds, _clients.size(), timeout);
    switch (pollResult)
    {
    case -1:
        logger.error("[POLLING] Multiplexer: poll() returned -1, error occurred: " + std::string(strerror(errno)));
        return -1;
    case 0:
        logger.log("[POLLING] Multiplexer: poll() returned 0, timeout occurred");
        return -1;
    default:
        logger.log("[POLLING] Multiplexer: " + std::to_string(_clients.size()) + " sockets are ready");
        return 0;
    }
}

/*
 * Gets the server that a client is connected to
 * @param socket The client socket
 * @return A reference to the server the client is connected to
 */
Socket &Multiplexer::_getSocketForClient(const int fd)
{
    std::vector<Socket>::iterator it;

    for (it = _sockets.begin(); it != _sockets.end(); it++)
    {
        if (std::find(it->getConnectedClients().begin(), it->getConnectedClients().end(), fd) !=
            it->getConnectedClients().end())
            return *it;
    }
    return *it;
}