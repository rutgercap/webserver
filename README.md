# webserver

This is a web server that behaves similar to NGINX, a popular open-source web server. The goal of this project was to get a thorough understanding of the HTTP protocol and to get familiar with the workings of webservers. 

## Features

* Supports HTTP/1.1
* Serve static files (HTML, CSS, JavaScript, images, etc.)
* Supports multiple virtual servers to reverse proxy requests to other servers
* Supports python CGI scripts


## Installation

Clone the repository:

```console

git clone https://github.com/[USERNAME]/nginx-like-web-server.git

```

Compile the webserver through the makefile:

```console

make

```

Run executable (optionally with path to a configuration file):

```console

./webserver [OPTIONAL: path-to-configfile] 

```


Or run with default configuration through the makefile:

```console

make run

```


## Configuration
The server can be configured using a config file. The path-to-config file has to be added while running the server.
Possible configurations:

* Choose ports and servernames of virtual servers
* Set up default error pages
* Limit client body size
* Set up routes with following possible configurations:
  * Define accepted HTTP methods
  * Set up HTTP redirections
  * Define root directory
  * Turn on/off directory listing
  * Setup index files
  * Execute CGI based on file extensions (currently only supports Python)
  
