#include "Multiplexer.hpp"
#include <Webserver.hpp>
#include <CGI.hpp>

int startWebserver(std::vector<Socket> sockets)
{
  Multiplexer multiplexer(sockets);
  try
  {
    multiplexer.waitForEvents(); // Start the multiplexer loop, does all the polling and handling of events etc.
  }
  catch (std::exception &e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, char **argv, char **env)
{
  (void)env;
  std::vector<Socket> sockets;
  /*  Input check */
  if (argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " <config_file>" << std::endl;
    return 1;
  }
  if (initWebserver(&sockets, argv[1]))
    return 1;
  return startWebserver(sockets);
}
