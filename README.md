# Webserv 

A lightweight HTTP web server build in c++ as part of a the 42 network common core 

## Compilation
packages needed for this project 
- `gnu make` - for running the makefile
- `g++/clang` - for compiling the source code

To compile:

```bash
make
```

---

## Usage

Run the server using the defualt config:
```bash
./webserve
```

Or specify a config file frim the `configtest` directory:

``` bash
./webserve confictest/FILE.conf
```

---

## The project requirements
For this project, we had to make an HTTP web server.
The server needs to handle GET, POST, and DELETE methods.
It must be able to serve static pages, send the right response codes with each response, and allow clients to upload and delete files.
All I/O operations must be non-blocking.
Every I/O operation must go through `epoll`, and the server will only use one epoll instance.
Multiple servers must be able to run simultaneously from a single config file.

## Server operation
The server starts by reading the config file and parsing its contents for later use.
It then sets up the the server epoll instance, creates the server sockets, and sets the socket file descriptors to be non-blocking.
Next, pipes for the standard output and error are created and added to the `epoll`.
Once everything is set up, the main `epoll_wait` loop begins, and the server listens for events.
When a client tries to connect, the connection is accepted through the server's listening socket.
The client is assigned its own file descriptor, which is set to non-blocking and added to the epoll instance. 
Its epoll event will be set to listen for incoming data.
The server reads the client's request and stores the necessary data. 
It then returns to the `epoll_wait` loop. This continues until the full request is received. 
At that point, the epoll event for the client is updated to indicate readiness for sending a response.
The server then validates the request and sends the appropriate response, or an error response if necessary.
Once the response is fully sent, the client’s file descriptor is removed from the epoll and closed.