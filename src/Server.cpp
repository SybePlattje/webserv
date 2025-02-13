#include "Server.hpp"
#include <sys/socket.h>
#include <stdexcept>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

Server::Server(std::string serverIP, int serverPort)
{
	fillErrorMap();
	if (createServerSocket(serverPort, serverIP) != 0)
		throw std::runtime_error("failed to setup server socket");
}

Server::~Server() {}

/**
 * @brief creates the epoll and starts the main loop for the server
 * 
 * @return 0 when doen,
 * @return -1 on error,
 * @return -2 on crititcla error
 */
int Server::setupEpoll()
{
	epoll_fd_ = epoll_create(MAX_EVENTS);
	if (epoll_fd_ == -1)
	{
		int nr = checkErrno(errno);
		closeFd(server_fd_);
		return nr;
	}
	epoll_event event{};
	event.events = EPOLLIN;
	event.data.fd = server_fd_;
	int nr = doEpollCtl(EPOLL_CTL_ADD, server_fd_, &event);
	if (nr != 0)
	{
		closeFd(epoll_fd_, server_fd_);
		return nr;
	}
	if (listenLoop() == -2)
		return -2;
	return 0;
}

//private functions

/**
 * @brief Sets the socket for the server up and makes it so it's up and running
 * 
 * @param port what port there will be listened on
 * @param serverIP the ip address of the server
 * @return 0 on success,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::createServerSocket(int port, const std::string& serverIP)
{
	server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd_ == -1)
		return 1;
	int opt = 1;
	setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	sockaddr_in serverAddr = setServerAddr(serverIP, port);
	int nr = bindServerSocket(serverAddr);
	if (nr != 0)
		return nr;
	nr = listenServer();
	if (nr != 0)
		return nr;
	setNonBlocking(server_fd_);
	return 0;
}

/**
 * @brief creates the socket address of the server 
 * 
 * @param serverIP the string of the ip address of the server
 * @param port on what port the server will be active
 * @return sockaddr_in server address
 */
sockaddr_in Server::setServerAddr(const std::string& serverIP, int port)
{
	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
	serverAddr.sin_port = htons(port);
	return serverAddr;
}

/**
 * @brief binds the server socket and the server address together
 * 
 * @param serverAddr the server socket address
 * @return 0 on success,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::bindServerSocket(sockaddr_in& serverAddr)
{
	if (bind(server_fd_, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		int nr = checkErrno(errno);
		closeFd(server_fd_);
		return nr;
	}
	return 0;
}

/**
 * @brief prepares the server to accept conections on the socked file descriptor
 * 
 * @return 0 on success,
 * @return -1 on error,
 * @return -2 on crititcal error
 */
int Server::listenServer()
{
	if(listen(server_fd_, SOMAXCONN) < 0)
	{
		int nr = checkErrno(errno);
		closeFd(server_fd_);
		return nr;
	}
	return 0;
}

/**
 * @brief sets the file descripter to be non blocking
 * 
 * @param fd file descriptor needed to be made non blocking
 */
void Server::setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief closes file descripters, sockets, epoll
 * 
 * @param fdA optinal file descripter A
 * @param fdB optinal file descripter B
 * @param fdC optinal file descripter C
 */
void Server::closeFd(int fd_a, int fd_b, int fd_c)
{
	if (fd_a != -1)
		close(fd_a);
	if (fd_b != -1)
		close(fd_b);
	if (fd_c != -1)
		close(fd_c);
}

/**
 * @brief loops through the events and handles the requests
 * 
 * @param epoll_fd_ the file descripter of the epoll where all events go through
 */
int Server::listenLoop()
{
	epoll_event events[MAX_EVENTS];
	std::string method, source, http_version;
	bool chunked = false;
	while (true)
	{
		int event_count = epoll_wait(epoll_fd_, events, MAX_EVENTS, TIMEOUT_MS);
		std::string request;
		for (int i = 0; i < event_count; ++i)
		{
			int nr = checkEvent(events[i], method, source, http_version, chunked);
			if (nr != 0)
				return nr;
		}
	}
	closeFd(epoll_fd_, server_fd_);
	return 0;
}

/**
 * @brief handles the specific event being requested
 * 
 * @param events the data of the curent event
 * @param epoll_fd_ the file descpriptor of epoll
 * @param method what method is being used 
 * @param source what end point was requested
 * @param http_version the http version
 * @param chunked if data is chunked or not
 * @return 0 on success,
 * @return -2 or error
 */
int Server::checkEvent(epoll_event events, std::string& method, std::string& source, std::string& http_version, bool& chunked)
{
	int fd = events.data.fd;
	if (fd == server_fd_) // new connection
	{
		if (setupConnection() == -2)
		{
			closeFd(epoll_fd_, server_fd_);
			return -2;
		}
	}
	else if (events.events & EPOLLIN) // read event
	{
		request_buffer_ = readRequest(fd, method, source, http_version, chunked);
		if (handleClient(fd, chunked) == -2)
		{
			closeFd(epoll_fd_, server_fd_);
			return -2;
		}
	}
	else if (events.events & EPOLLOUT) // write event
		handleResponse(fd, &events, method, source, http_version, chunked);
	return 0;
}

/**
 * @brief adds the new client to the epoll
 * 
 * @return 0 when everything is set up,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::setupConnection()
{
	sockaddr_in clientAddr{};
	socklen_t clientLen = sizeof(clientAddr);
	int clientFd = accept(server_fd_, (sockaddr*)&clientAddr, &clientLen);
	if (clientFd != -1)
	{
		setNonBlocking(clientFd);
		epoll_event client_event{};
		client_event.events = EPOLLIN;
		client_event.data.fd = clientFd;
		int nr = doEpollCtl(EPOLL_CTL_ADD, clientFd, &client_event);
		if (nr != 0)
				return nr;
		return 0;
	}
	else
		return checkErrno(errno);
}

/**
 * @brief checks what error happed and sets the corresponding return value
 * 
 * @param err the errno
 * @param epoll_fd_ file descriptor of the epoll
 * @param fd file descriptor of the client
 * @param events the events of the current client
 * @return 0 when able to save,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::checkErrno(int err, int fd, epoll_event events)
{
	std::unordered_map<int, e_ErrorInfo>::iterator it = error_map_.find(err);
	if (it != error_map_.end())
	{
		std::cerr << error_map_.at(err).message << std::endl;
		if (err == EEXIST)
		{
			if (doEpollCtl(EPOLL_CTL_MOD, fd, &events) != 0)
				return it->second.returnValue;
			return 0;
		}
		else if (err == ENOENT)
		{
			if (doEpollCtl(EPOLL_CTL_ADD, fd, &events) != 0)
				return it->second.returnValue;
			return 0;
		}
		else if (err == 0) {}
		return it->second.returnValue;
	}
	std::cerr << "Unknown error: " << strerror(err) << std::endl;
	return -1;
}

/**
 * @brief fills the errorMap with the errno, message, and return value
 * 
 */
void Server::fillErrorMap()
{
	error_map_ = {
		{EAGAIN, 		{"No pending connection, just return.", -1}},
		{ECONNABORTED,	{"Connection aborted before accept(), Ignoring", -1}},
		{EMFILE,		{"Too many open files. Consider increasing file descriptor limits.", -1}},
		{ENFILE,		{"Too many open files. Consider increasing file descriptor limits.", -1}},
		{ENOMEM,		{"System out of memory. Cannot accept new connections.", -2}},
		{EBADF,			{"Critical error: Invalid socket or file descpritor. Restarting server may be required.", -2}},
		{EINVAL,		{"Critical error: Invalid socket or arguments. Restarting server may be required.", -2}},
		{ENOSPC,		{"Reached system limit for epoll FDs. Consider increasing limit.", -2}},
		{EEXIST,		{"FD already in epoll. Modifying instead.", -1}},
		{ENOENT,		{"FD does not exist. Retrying with EPOLL_CTL_ADD", -1}},
		{EADDRINUSE, 	{"Port is already in use. Try another port or wait.", -1}},
		{EADDRNOTAVAIL,	{"The requested address is not available on this machine.", -2}},
		{EAFNOSUPPORT,	{"Address family not supported.", -2}},
		{ENOTSOCK, 		{"The file descriptor is not a socket.", -2}},
		{EACCES,		{"Permission denied. Try using a different port or running as root.", -2}},
		{EPERM,			{"Operation not permitted on socket or FD.", -2}},
		{ENOBUFS,		{"Insufficient resources to complete the operation.", -2}},
		{EFAULT,		{"Invalid memory address provided for sockaddr or read.", -2}},
		{EOPNOTSUPP,	{"Operation not supported on this socket type. Check socket configuration.", -2}},
		{EWOULDBLOCK,	{"Resource temporarily unavailable (try again).", -1}},
		{ECONNRESET,	{"Connection reset by peer. The connection was forcibly closed.", -1}},
		{EINTR,			{"Operation interrupted by signal, try again.", -1}},
		{EIO,			{"I/O error occurred during read operation.", -2}},
		{ESHUTDOWN,		{"Socket has been shut down; no further reading possible.", -1}}
	};
}

/**
 * @brief handling the read event of the client
 * 
 * @param client_fd the file descriptor of the client
 * @param chunked if the response needs to be chunked
 * @return  0 when done with the read event,
 * @return -1 on error,
 * @return -2 on cirtical error
 */
int Server::handleClient(int client_fd, bool& chunked)
{
	epoll_event event{};
	event.events = EPOLLOUT;
	event.data.fd = client_fd;
	if (!request_buffer_.empty())
		return doClientModification(client_fd, &event, "500 Internal Server Error", "./example/errorPages/500.html", chunked);
	else
		return doClientDelete(client_fd, "500 Internal Server Error", "./example/errorPages/500.html", &event, chunked);
}

/**
 * @brief tries to do the EPOLL_CTL_MOD operation on the client_fd
 * 
 * @param client_fd the file descriptor we try to modify in epoll
 * @param event the events of the client
 * @param status the status code for the response of failure
 * @param location the location of the error page
 * @param chunked if data send back is chunked 
 * @return 0 on success,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::doClientModification(int client_fd, epoll_event* event, const std::string& status, const std::string& location, bool& chunked)
{
	int nr = doEpollCtl(EPOLL_CTL_MOD, client_fd, event);
	if (nr != 0)
	{
		doClientDelete(client_fd, status, location, event, chunked);
		return nr;
	}
	return 0;
}

/**
 * @brief tries to di the EPOLL_CTL_DEL operation on the clientFd
 * 
 * @param client_fd the file descriptor we try to delete out of epoll
 * @param status the status code we send to the client
 * @param location the location of the error page
 * @param event the events of the client
 * @param chunked if the data send back is chunked
 * @return -2 when done
 */
int Server::doClientDelete(int client_fd, const std::string& status, const std::string location, epoll_event* event, bool& chunked)
{
	sendResponse(client_fd, status, location, chunked);
	doEpollCtl(EPOLL_CTL_DEL, client_fd, event);
	return -2;
}

/**
 * @brief reads the data the client sends and if needed un chunks the body
 * 
 * @param client_fd the file descriptor of the client
 * @param method will hold what method was used for this request
 * @param source will hold the end point requested by the client
 * @param httpVersion will hold what http version the client used
 * @param chunked will be true if the response needs to be chunked
 * @return the parsed body 
 */
std::string Server::readRequest(int client_fd, std::string& method, std::string& source, std::string& httpVersion, bool& chunked)
{
	char buffer[BUFFER_SIZE];
	std::string request_buffer;
	ssize_t bytes_received;
	while((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0)
	{
		request_buffer.append(buffer, bytes_received);
		// check if headers are fully received
		size_t headerEnd = request_buffer.find("\r\n\r\n");
		if (headerEnd != std::string::npos)
		{
			setMethodSourceHttpVerion(method, source, httpVersion, request_buffer);
			std::string headers = request_buffer.substr(0, headerEnd);
			std::string body;
			size_t bodyStart = headerEnd + 4; // Skip \r\n\r\n
			// check if it's chunked transfer encoding
			if (headers.find("Transfer-Encoding: chunked") != std::string::npos || headers.find("TE: chunked") != std::string::npos)
			{
				chunked = true;
				return handleChunkedRequest(bodyStart, request_buffer, client_fd, buffer);
			}
			// Handle Content-Length
			size_t contentLengthPos = headers.find("Content-Length: ");
			if (contentLengthPos != std::string::npos)
			{
				return handleContentLength(contentLengthPos, headers, request_buffer, bodyStart, client_fd, buffer);
			}
			return request_buffer;
		}
	}
	return "";
}

/**
 * @brief sets the method, source, and httpversion to that what the client used
 * 
 * @param method will hold what method was used for this request
 * @param source will hold the end point requested by the client
 * @param http_version will hold what http version the client used
 * @param request_buffer the string with the request header
 */
void Server::setMethodSourceHttpVerion(std::string& method, std::string& source, std::string& http_version, std::string& request_buffer)
{
	std::istringstream stream(request_buffer);
	stream >> method >> source >> http_version;
}

/**
 * @brief reads into the request from the client
 * 
 * @param client_fd the client file descriptor
 * @param buffer the buffer being filled by reading
 * @param request_buffer the request from the client
 * @return 0 on success,
 * @return -1 on error
 */
int Server::useRevc(int client_fd, char buffer[], std::string& request_buffer)
{
	size_t bytes_recieved = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
	if (bytes_recieved <= 0)
	{
		return -1;
	}
	try
	{
		request_buffer.append(buffer, bytes_recieved);
	}
	catch (std::length_error& e)
	{
		(void)e;
		return -1;
	}
	return 0;
}

/**
 * @brief dechunks the request body from the client
 * 
 * @param body_start the index where the request body starts in requestBuffer
 * @param request_buffer the string holding the request from the client
 * @param client_fd the file desciptor of the client
 * @param buffer the buffer what hold the read data
 * @return the dechunked request body
 */
std::string Server::handleChunkedRequest(size_t body_start, std::string& request_buffer, int client_fd, char buffer[])
{
	std::string decoded_body;
	size_t pos = body_start;
	while (true)
	{
		// read chunk size
		size_t chunk_size_end = request_buffer.find("\r\n", pos);
		if (chunk_size_end == std::string::npos)
		{
			if (useRevc(client_fd, buffer, request_buffer) == -1)
			{
				break;
			}
			continue;
		}
		std::string chunkSizeHex = request_buffer.substr(pos, chunk_size_end - pos);
		int chunk_size = std::stoi(chunkSizeHex, nullptr, 16);
		pos = chunk_size_end + 2; // move past \r\n
		if (chunk_size == 0) break; // end of chunks
		// Ensure full chunk is recieved
		while (request_buffer.size() < pos + chunk_size + 2)
		{
			if (useRevc(client_fd, buffer, request_buffer) == -1)
			{
				return "";
			}
		}
		// Extract chunk data
		std::string chunk_data = request_buffer.substr(pos, chunk_size);
		decoded_body.append(chunk_data);
		pos += chunk_size + 2; // Move past \r\n
	}
	return decoded_body;
}

/**
 * @brief handles the request with a content length
 * 
 * @param content_length_pos the index of where the content length is located in the requestBuffer
 * @param headers the header of the response
 * @param request_buffer the entire response string
 * @param body_start the index of where the body starts
 * @param client_fd the client file descriptor
 * @param buffer the buffer used for reading
 * @return returns the body base on the length given in contentLength
 */
std::string Server::handleContentLength(size_t content_length_pos, std::string& headers, std::string& request_buffer, size_t body_start, int client_fd, char buffer[])
{
	size_t start = content_length_pos + 16; // skipping "Content-Length: "
	size_t end = headers.find("\r\n", start);
	int contentLength = std::stoi(headers.substr(start, end - start));
	while (request_buffer.size() < body_start + contentLength)
	{
		if(useRevc(client_fd, buffer, request_buffer) == -1)
		{
			break;
		}
	}
	return request_buffer.substr(body_start, contentLength);
}

/**
 * @brief handles the respose to the client
 * 
 * @param client_fd the file descriptor of the client
 * @param event the events of the client
 * @param method what method is used by the client
 * @param source wheat source is used by the client
 * @param http_version what http version is used by the client
 * @param chunked if the response needs to be chunked or not
 */
void Server::handleResponse(int client_fd, epoll_event* event, std::string& method, std::string& source, std::string& http_version, bool& chunked)
{
	std::string file_path;
	if (source == "/")
	{
		file_path = "./example/index.html"; // change later to file from location
	}
	else if (source == "/main.css")
	{
		file_path = "./example/errorPages/main.css";
	}
	else
	{
		file_path = source;
	}
	// if auto indexing is on and standard file doesn't exist it should show folder structure from that location
	if (http_version != "HTTP/1.1")
	{
		sendResponse(client_fd, "505 HTTP Version Not Supported", "./example/errorPages/505.html", chunked); // change later to predefined locations or use fallbacks
	}
	if (method != "GET" && method != "POST") // change later to check against alowed methods from location
	{
		sendResponse(client_fd, "400 Bad Request", "./example/errorPages/400.html", chunked);
		doEpollCtl(EPOLL_CTL_DEL, client_fd, event);
		close(client_fd);
		return;
	}
	// if method is post and client_max_body_size is set check if request is bigger that. Return 413 if true
	if (fileExists(file_path))
	{
		if (filePermission(file_path))
		{
			sendResponse(client_fd, "200 OK", file_path, chunked);
		}
		else
		{
			sendResponse(client_fd, "403 Forbidden", "./example/errorPages/403.html", chunked);
		}
	}
	else
	{
		sendResponse(client_fd, "404 Not Found", "./example/errorPages/404.html", chunked);
	}
	doEpollCtl(EPOLL_CTL_DEL, client_fd, event);
	closeFd(client_fd);
	return;
}

/**
 * @brief makes the response and sends it
 * 
 * @param client_fd the file descriptor of the client who gets the response
 * @param status the status of the response
 * @param file_location the file location of the response
 * @param chunked if the response needs to be chunked
 */

void Server::sendResponse(int client_fd, const std::string& status, const std::string& file_location, bool& chunked)
{
	std::ostringstream response;
	response << "HTTP/1.1 " << status << "\r\n";
	response << "Connection: close\r\n";

	std::ifstream file_stream(file_location, std::ios::binary);
	if (!file_stream.is_open())
	{
		std::cerr << "files_stream open: " << file_location << std::endl;
		return;
	}

	response << "Content-Type: " << getContentType(file_location) << "\r\n";

	if (chunked)
	{
		response << "Transfer-Encoding: chunked\r\n\r\n";
		send(client_fd, response.str().c_str(), response.str().size(), 0);
		sendChunkedResponse(client_fd, file_stream);
		chunked = false;
	}
	else
	{
		file_stream.seekg(0, std::ios::end);
		std::streamsize file_size = file_stream.tellg();
		file_stream.seekg(0, std::ios::beg);
		response << "Content-Length: " << file_size << "\r\n\r\n";
		send(client_fd, response.str().c_str(), response.str().size(), 0);
		sendFile(client_fd, file_stream);
	}
}

/**
 * @brief sends back a chunked response
 * 
 * @param client_Fd the file descriptor of the client
 * @param file_stream the file stream of the response needed to send
 */
void Server::sendChunkedResponse(int client_Fd, std::ifstream& file_stream)
{
	char buffer[BUFFER_SIZE];
	while(file_stream.read(buffer, BUFFER_SIZE) || file_stream.gcount() > 0)
	{
		std::ostringstream chunk;
		chunk << std::hex << file_stream.gcount() << "\r\n"; // chunk size in hex
		send(client_Fd, chunk.str().c_str(), chunk.str().size(), 0);
		send(client_Fd, buffer, file_stream.gcount(), 0);
		send(client_Fd, "\r\n", 2, 0);
	}
	send(client_Fd, "0\r\n\r\n", 5, 0); // Last chunk (size 0) indicates end of transfer
}

/**
 * @brief sends back non chunked response
 * 
 * @param client_fd the file descriptor of the client
 * @param file_stream the file stream of the file being send 
 */
void Server::sendFile(int client_fd, std::ifstream& file_stream)
{
	char buffer[BUFFER_SIZE];
	while (file_stream.read(buffer, BUFFER_SIZE) || file_stream.gcount() > 0)
	{
		send(client_fd, buffer, file_stream.gcount(), 0);
	}
}

/**
 * @brief checks what kind of file type we send back to the client
 * 
 * @param file_path the file with the content we want to send
 * @return the content type we send to the client
 */
std::string Server::getContentType(const std::string& file_path)
{
	size_t dot_pos = file_path.rfind('.');
	if (dot_pos == std::string::npos)
		return "application/octet-stream"; // Default binary type
	std::string extension = file_path.substr(dot_pos);
	if (extension == ".html")
		return "text/html";
	if (extension == ".css")
		return "text/css";
	if (extension == ".js")
		return "application/javascript";
	if (extension == ".json")
		return "application/json";
	if (extension == ".png")
		return "image/png";
	if (extension == ".jpg" || extension == ".jpeg")
		return "image/jpeg";
	if (extension == ".gif")
		return "image/gif";
	if (extension == ".svg")
		return "image/svg+xml";
	if (extension == ".txt")
		return "text/plain";
	return "application/octet-stream"; // Default for unknown types
}

/**
 * @brief checks if the file exists
 * 
 * @param path the path to the file
 * @return true if file exists,
 * @return false if file doesn't exists
 */
bool Server::fileExists(const std::string& path)
{
	struct stat buffer;
	return stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode);
}

/**
 * @brief checks if ohters have read permission of file
 * 
 * @param path the path to the file to check permission
 * @return true if file others has read permission,
 * @return false if file others hasn't read permission
 */
bool Server::filePermission(const std::string& path)
{
	struct stat buffer;
	stat(path.c_str(), &buffer);
	return buffer.st_mode & S_IROTH;
}

/**
 * @brief does an action with epoll_ctl. Be it add a new client to it, modify it, or delete it
 * 
 * @param mode what action we take in epoll_ctl
 * @param client_fd the client file despcriptor
 * @param event the events of the client
 * @return 0 when done,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::doEpollCtl(int mode, int client_fd, epoll_event* event)
{
	if (epoll_ctl(epoll_fd_, mode, client_fd, event) == -1)
	{
		int nr = checkErrno(errno, client_fd, *event);
		if (nr < 0)
			return nr;
	}
	return 0;
}