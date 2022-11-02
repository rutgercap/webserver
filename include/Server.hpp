#pragma once
#include <string>
#include <vector>
#include <Route.hpp>
#include <Logger.hpp>

/*
In the configuration file, you should be able to:
• Choose the port and host of each ’server’.
• Setup the server_names or not.
• The first server for a host:port will be the default for this host:port (that means it will answer to all the requests that don’t belong to an other server).
• Setup default error pages.
• Limit client body size.
• Setup routes with one or multiple of the following rules/configuration (routes wont be using regexp):
	◦ Define a list of accepted HTTP methods for the route.
	◦ Define a HTTP redirection.
	◦ Define a directory or a file from where the file should be searched (for example, if url /kapouet is rooted to /tmp/www, url /kapouet/pouic/toto/pouet is /tmp/www/pouic/toto/pouet).
	◦ Turn on or off directory listing.
	◦ Set a default file to answer if the request is a directory.
	◦ Execute CGI based on certain file extension (for example .php).
	◦ Make the route able to accept uploaded files and configure where they should be saved.
		∗ Do you wonder what a CGI is?
		∗ Because you won’t call the CGI directly, use the full path as PATH_INFO.
		∗ Just remember that, for chunked request, your server needs to unchunked it and the CGI will expect EOF as end of the body.
		∗ Same things for the output of the CGI. If no content_length is returned from the CGI, EOF will mark the end of the returned data.
		∗ Your program should call the CGI with the file requested as first argument.
		∗ The CGI should be run in the correct directory for relative path file access.
		∗ Your server should work with one CGI (php-CGI, Python, and so forth).
You must provide some configuration files and default basic files to test and demon- strate every feature works during evaluation.
*/

#define MAX_PORT 65535

class Server {
	public:
		Server();
		~Server();

		// To check if variables have been set
		bool	hasServerName() const;
		bool	hasHost() const;
		bool	hasPort() const;
		bool	hasMaxBody() const;
		bool	hasErrorPage() const;
		bool	hasRoutes() const;

		//  Getters
		std::string		&getServerName() const;
		std::string		&getHost() const;
		std::string		&getErrorPage() const;
		unsigned int	&getMaxBody() const;
		int				&getPort() const;

		// Setters
		void	setServerName(std::string const& value);
		void	setHost(std::string const& value);
		void	setErrorPage(std::string const& value);
		int		setPort(int const& value);
		int		setMaxBody(double const& value);

	private:
		std::string*		_serverName;
		std::string*		_host;
		int*				_port;
		unsigned int* 		_maxBodySize;
		std::string*		_defaultErrorPage;
		std::vector<Route>	_routes;
};