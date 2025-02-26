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
#include <filesystem>
#include <sys/stat.h>
#include <dirent.h>

Server::Server(Config& config) : config_(config)
{
    fillErrorMap();
    std::string server_name = config_.getServerName();
    if (server_name == "localhost")
        server_name = "127.0.0.1";
    main_index_ = config_.getIndex();
    root_path_ = config_.getRoot();
    locations_ = config.getLocations();
    error_pages_ = config.getErrorPages();
    if (createServerSocket(config.getListen(), server_name) != 0)
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

    nr = setupPipe();
    if (nr < 0)
    {
        closeFd(epoll_fd_, server_fd_);
        return nr;
    }

    nr = putCoutCerrInEpoll();
    if (nr < 0)
    {
        closeFd(epoll_fd_, server_fd_);
        return nr;
    }

    nr = listenLoop();
    if (nr < 0)
    {
        closeFd(stdout_pipe_[0], stderr_pipe_[0]);
        closeFd(epoll_fd_, server_fd_);
        return nr;
    }

    closeFd(stdout_pipe_[0], stderr_pipe_[0]);
    return 0;
}

//private functions

/**
 * @brief puts the standard outpute and standard error file descriptor in the epoll
 * 
 * @return 0 when done,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::putCoutCerrInEpoll()
{
    struct epoll_event std_event;
    std_event.events = EPOLLIN;
    std_event.data.fd = stdout_pipe_[0];
    int nr = doEpollCtl(EPOLL_CTL_ADD, stdout_pipe_[0], &std_event);
    if (nr < 0)
        return nr;

    std_event.data.fd = stderr_pipe_[0];
    nr = doEpollCtl(EPOLL_CTL_ADD, stderr_pipe_[0], &std_event);
    if (nr < 0)
        return nr;
    return 0;
}

/**
 * @brief makes the pipes for the standard output and the standard error so we can capture it with epoll
 * 
 * @return 0 when done,
 * @return -1 on pipe error
 */
int Server::setupPipe()
{
    if (pipe(stdout_pipe_) == -1 || pipe(stderr_pipe_) == -1)
        return -1;
	
    setNonBlocking(stdout_pipe_[0]);
    setNonBlocking(stdout_pipe_[1]);
    setNonBlocking(stderr_pipe_[0]);
    setNonBlocking(stderr_pipe_[1]);

    dup2(stdout_pipe_[1], STDOUT_FILENO);
    dup2(stderr_pipe_[1], STDERR_FILENO);

    closeFd(stdout_pipe_[1], stderr_pipe_[1]);

    std::cout.setf(std::ios::unitbuf);
    return 0;
}

/**
 * @brief Sets the socket for the server up and makes it so it's up and running
 * 
 * @param port what port there will be listened on
 * @param server_ip the ip address of the server
 * @return 0 on success,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::createServerSocket(uint port, const std::string& server_ip)
{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1)
        return 1;
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in server_addr = setServerAddr(server_ip, port);
    int nr = bindServerSocket(server_addr);
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
 * @param server_ip the string of the ip address of the server
 * @param port on what port the server will be active
 * @return sockaddr_in server address
 */
sockaddr_in Server::setServerAddr(const std::string& server_ip, uint port)
{
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr.sin_port = htons(port);
    return server_addr;
}

/**
 * @brief binds the server socket and the server address together
 * 
 * @param server_addr the server socket address
 * @return 0 on success,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::bindServerSocket(sockaddr_in& server_addr)
{
    if (bind(server_fd_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0)
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
 * @param fd_a optinal file descripter A
 * @param fd_b optinal file descripter B
 * @param fd_c optinal file descripter C
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
    std::string method, source, http_version = "";
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
        bool max_size = false;
        request_buffer_ = readRequest(fd, method, source, http_version, chunked, max_size);
        if (max_size)
            return setupResponse(fd, "413 Length Too Large", chunked, &events, error_pages_, 413);
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
        std::cerr << it->second.message << std::endl;
        if (err == EEXIST)
        {
            if (doEpollCtl(EPOLL_CTL_MOD, fd, &events) != 0)
                return it->second.return_value;
            return 0;
        }
        else if (err == ENOENT)
        {
            if (doEpollCtl(EPOLL_CTL_ADD, fd, &events) != 0)
                return it->second.return_value;
            return 0;
        }
        else if (err == 0) {}
        return it->second.return_value;
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
    if (client_fd == stdout_pipe_[0] || client_fd == stderr_pipe_[0])
        return 0;
    epoll_event event{};
    event.events = EPOLLOUT;
    event.data.fd = client_fd;
    if (!request_buffer_.empty())
    {
        // std::cout << "handle client request buffer not empty" << std::endl;
        return doClientModification(client_fd, &event, "500 Internal Server Error", "./example/errorPages/500.html", chunked);
    }
    else
    {
        std::cerr << "handle client request buffer empty\n";
        return doClientDelete(client_fd, "500 Internal Server Error", "./example/errorPages/500.html", &event, chunked);
    }
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
 * @brief tries to do the EPOLL_CTL_DEL operation on the clientFd
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
 * @brief logs the messages of cout and cerr and prints them to the logs
 * 
 * @param msg the message to be printed
 * @param fd the current file discriptor
 */
void Server::logMsg(const char *msg, int fd)
{
    std::string file;
    if (fd == stdout_pipe_[0])
        file = STANDARD_LOG_FILE;
    else
        file = STANDARD_ERROR_LOG_FILE;
    if (mkdir("logs", 0777) < 0 && errno != EEXIST)
    {
        std::cerr << "mkdir() error: " << strerror(errno) << std::endl;
        return;
    }
    file.insert(0, "logs/");
    int file_fd = open(file.c_str(), O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);
    write(file_fd, msg, strlen(msg));
    close(file_fd);
}

/**
 * @brief reads the data the client sends and if needed un chunks the body
 * 
 * @param client_fd the file descriptor of the client
 * @param method will hold what method was used for this request
 * @param source will hold the end point requested by the client
 * @param http_version will hold what http version the client used
 * @param chunked will be true if the response needs to be chunked
 * @return the parsed body 
 */
std::string Server::readRequest(int client_fd, std::string& method, std::string& source, std::string& http_version, bool& chunked, bool& max_size)
{
    char buffer[BUFFER_SIZE];
    std::string request_buffer;
    ssize_t bytes_received = 0;

    if (client_fd == stdout_pipe_[0] || client_fd == stderr_pipe_[0])
        return handleCoutErrOutput(client_fd);

    while((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        request_buffer.append(buffer, bytes_received);
        // check if headers are fully received
        size_t header_end = request_buffer.find("\r\n\r\n");
        if (header_end != std::string::npos)
            return readHeader(request_buffer, header_end, method, source, http_version, chunked, client_fd, buffer, max_size);
	}
	std::cerr << "read request empty at end\n";
	return "";
}

std::string Server::readHeader(std::string request_buffer, size_t header_end, std::string& method, std::string& source, std::string http_version, bool& chunked, int client_fd, char buffer[], bool& max_size)
{
     std::cout << request_buffer << std::endl;
    setContentTypeRequest(request_buffer, header_end);

    setMethodSourceHttpVerion(method, source, http_version, request_buffer);

    std::string headers = request_buffer.substr(0, header_end);
    request_[REQUEST_HEADER] = headers;

    size_t body_start = header_end + 4; // Skip \r\n\r\n

    // check if it's chunked transfer encoding
    if (headers.find("Transfer-Encoding: chunked") != std::string::npos || headers.find("TE: chunked") != std::string::npos)
    {
        chunked = true;
        return handleChunkedRequest(body_start, request_buffer, client_fd, buffer);
    }
    // Handle Content-Length
    size_t content_length_pos = headers.find("Content-Length: ");
    if (content_length_pos != std::string::npos)
    {
        std:: string content_body = handleContentLength(content_length_pos, headers, request_buffer, body_start, client_fd, buffer, max_size);
        if (max_size)
            return "";
        else
            return content_body;
    }
    return request_buffer;
}

/**
 * @brief looks into the client request and sets the Content-Type
 * 
 * @param request_buffer th string holding the request from the client
 * @param header_end position of the end of the request header
 * @return 0 when done,
 * @return -1 if request_buffer doesn't have "Content-Type: "
 */
int Server::setContentTypeRequest(std::string request_buffer, size_t header_end)
{
    size_t request_type_pos= request_buffer.find("Content-Type: ");
    if (request_type_pos != std::string::npos)
    {
        request_type_pos += 14; // skip past "Content-type: "
        size_t end = request_buffer.substr(request_type_pos, header_end - request_type_pos).length();
        request_[REQUEST_TYPE] = request_buffer.substr(request_type_pos, end);
        return 0;
    }
    return -1;
}

/**
 * @brief reads into the buffer of what is needed to be printed to the logs
 * 
 * @param client_fd the file discriptor of the standard output or standard error
 * @return empty string when done
 */
std::string Server::handleCoutErrOutput(int client_fd)
{  
    ssize_t bytes_received = 0;
    std::string request_buffer;
    char buffer[BUFFER_SIZE];
    if (client_fd == stdout_pipe_[0])
    {
        while ((bytes_received = read(client_fd, buffer, BUFFER_SIZE -1)) > 0)
        {
            buffer[bytes_received] = '\0';
            request_buffer.append(buffer, bytes_received);
        }
        request_buffer.insert(0, "[Captured stdcout: ]");
        logMsg(request_buffer.c_str(), client_fd);
        return "";
    }
    else
    {
        while ((bytes_received = read(client_fd, buffer, BUFFER_SIZE -1)) > 0)
        {
            buffer[bytes_received] = '\0';
            request_buffer.append(buffer, bytes_received);
        }
        request_buffer.insert(0, "[Captured stdcerr]: ");
        logMsg(request_buffer.c_str(), client_fd);
        return "";
    }
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
        return -1;
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
            if (useRevc(client_fd, buffer, request_buffer) == -1) break;
            continue;
        }
        std::string chunkSizeHex = request_buffer.substr(pos, chunk_size_end - pos);
        int chunk_size = std::stoi(chunkSizeHex, nullptr, 16);
        pos = chunk_size_end + 2; // move past \r\n
        if (chunk_size == 0) break; // end of chunks
        // Ensure full chunk is recieved
        while (request_buffer.size() < pos + chunk_size + 2)
            if (useRevc(client_fd, buffer, request_buffer) == -1) return "";
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
std::string Server::handleContentLength(size_t content_length_pos, std::string& headers, std::string& request_buffer, size_t body_start, int client_fd, char buffer[], bool& max_size)
{
    size_t start = content_length_pos + 16; // skipping "Content-Length: "
    size_t end = headers.find("\r\n", start);
	unsigned long contentLength = std::stoul(headers.substr(start, end - start));
    if (config_.getClientMaxBodySize() > 0)
    {
        if (contentLength > static_cast<unsigned long>(config_.getClientMaxBodySize()))
        {
            max_size = true;
            return "";
        }
    }
    while (request_buffer.size() < body_start + contentLength)
        if(useRevc(client_fd, buffer, request_buffer) == -1) break;
    request_[REQUEST_BODY].append(request_buffer.substr(body_start, contentLength));
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
 * @return 0 when client response has been send,
 * @return -1 on errors
 */
int Server::handleResponse(int client_fd, epoll_event* event, std::string& method, std::string& source, std::string& http_version, bool& chunked)
{
    std::string file_path;
    std::vector<Location>::iterator location_it = locations_.begin();

    int nr = checkHTTPVersion(http_version, client_fd, chunked, event);
    if (nr != 1)
        return nr;
	
    std::vector<std::string> token_location = sourceChunker(source);
    nr = checkLocations(token_location, file_path, client_fd, chunked, event, location_it);
    if (nr != 1)
        return nr;

    nr = checkAllowedMethods(location_it, client_fd, chunked, event, method);
    if (nr != 1)
        return nr;

    nr = checkFile(file_path, client_fd, chunked, event, location_it);
    if (nr < 0)
        return nr;
    else if (nr == 0)
    {
       nr = checkAutoIndexing(location_it, client_fd, chunked, event);
       if (nr != 1)
        return nr;
    }
    return 0;
}

int Server::checkAutoIndexing(std::vector<Location>::iterator location_it, int client_fd, bool& chunked, epoll_event* event)
{
    std::string path = config_.getRoot() + location_it->getPath();
    if (!isDirectory(path))
        return fileResponseSetup(client_fd, "404 Not Found", chunked, event, 404);
    if (!filePermission(path))
        return fileResponseSetup(client_fd, "404 Not Found", chunked, event, 404);

    std::string body;
    if (buildDirectoryResponse(path, body) == -1)
        return -1;
    
    if (sendResponse(client_fd, "200 OK", body, chunked, true) == -1)
        return -1;
    doEpollCtl(EPOLL_CTL_DEL, client_fd, event);
    close(client_fd);
    return 1;
}

bool Server::isDirectory(std::string& path)
{
    return std::filesystem::is_directory(path);
}

int Server::buildDirectoryResponse(const std::string& path, std::string& body)
{
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr)
    { 
        std::cerr << "failed to open directory!\n";
        return -1;
    }
    body.append("<!DOCTYPE html><html><body><h1>Direcotry Listing for ");
    body.append(path);
    body.append("</h1><ul>");
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        body.append("<li>");
        body.append(entry->d_name);
        body.append("</li>");
    }

    body.append("</ul></body></html>");
    closedir(dir);
    return 0;
}

/**
 * @brief checks if the client used the HTTP/1.1 version
 * 
 * @param http_version the HTTP version the client used
 * @param client_fd the file descriptor of the client
 * @param chunked flag if the response is chunked or not
 * @param event the events of the current client
 * @return 1 if version is correct,
 * @return 0 if version was incorect and response was send to the client,
 * @return -1 if the version was incorrect and the send response to the client failed
 */
int Server::checkHTTPVersion(std::string& http_version, int client_fd, bool& chunked, epoll_event* event)
{
    if (http_version != "HTTP/1.1")
    {
        return setupResponse(client_fd, "505 HTTP ersion Not Supported", chunked, event, error_pages_, 505);
    }
	return 1;
}

std::vector<std::string> Server::sourceChunker(std::string source)
{
    char delimiter = '/';
    std::vector<std::string> result;
    std::string token;
    if (source == "/")
    {
        result.push_back("/");
        return result;
    }
    for (char ch : source) 
    {
        if (ch == delimiter) 
        {
            if (!token.empty()) 
            {
                result.push_back(token);
                token.clear();
            }
        } 
        else
            token += ch;
    }
    if (!token.empty())
        result.push_back(token);
    return result;
}

/**
 * @brief checks if the location the client wants is known by us as by the config file
 * 
 * @param location_it begin of the vector holding the locations info
 * @param location_ite end of the vector holding the locations info
 * @param file_path the end file we will use to send to the client
 * @param source the location requested by the client
 * @param client_fd file descriptor of the client
 * @param chunked flag if the response is chunked or not
 * @param event the events of the client
 * @return 1 when location in source is found,
 * @return 0, if location was not found, but response was send to client
 * @return -1 if location was not found, and response to the client failed
 */
int Server::checkLocations(std::vector<std::string>& token_location, std::string& file_path, int client_fd, bool& chunked, epoll_event* event, std::vector<Location>::iterator& location_it)
{
    std::map<size_t, std::string> found_location;

    size_t token_size = token_location.size();
    setPosibleLocations(token_size, token_location, found_location);

    if (!found_location.empty())
    {
        size_t most_precise = 0;
        std::map<size_t, std::string>::iterator fl_it = found_location.begin();
        std::map<size_t, std::string>::iterator fl_ite = found_location.end();
        while (fl_it != fl_ite)
        {
            if (fl_it->first > most_precise)
                most_precise = fl_it->first;
            ++fl_it;
        }

        for (size_t i = 0; i < most_precise; ++i)
        {
            try 
            {
                if (found_location.at(i).compare("return"))
                    return setupResponse(client_fd, "301 Moved Permanently", chunked, event, error_pages_, 301);
            } catch (std::exception& e) {(void)e;}
        }
        std::string full_request;
        if (token_size == 1 && token_location.at(0) == "/")
            full_request = "/";
        else
            for (size_t l = 0; l < token_size; ++l)
                full_request.append("/" + token_location.at(l));
        if (found_location.at(most_precise) == full_request || found_location.at(most_precise).find(".") != std::string::npos)
        {
            location_it = std::next(locations_.begin(), most_precise);
            if (location_it->getPath() == "/")
                file_path = config_.getRoot() + config_.getIndex();
            else
                file_path = config_ .getRoot() + location_it->getIndex();
        }
    }
    if (file_path.empty())
    {
        if (setupResponse(client_fd, "404 Not Found", chunked, event, error_pages_, 404) == -1)
            return -1;
        return 0;
    }
    return 1;
}

void Server::setPosibleLocations(size_t token_size, std::vector<std::string>& token_location, std::map<size_t, std::string>& found_location)
{
    size_t location_size = locations_.size();
    if (token_size > 0 && token_location.at(token_size -1).find(".") != std::string::npos)
    {
        token_location.insert(token_location.begin(), token_location.at(token_size - 1));
        ++token_size;
    }
    if (token_size == 1 && token_location.at(0) == "/")
        found_location.insert(std::pair<size_t, std::string>(0, "/"));
    else
    {
        for (size_t i = 0; i < token_size; ++i)
        {
            size_t current = token_size - i;
            std::string loc = "";
            for (size_t j = 0; j < current; ++j)
                loc.append("/" + token_location.at(j));
            for (size_t k = 0; k < location_size; ++k)
            {
                try
                {
                    if (locations_.at(k).getPath().compare(loc) == 0)
                        found_location.insert(std::pair<size_t, std::string>(k, locations_.at(k).getPath()));
                }
                catch (std::exception& e) {(void)e;}
            }
        }
    }
}

/**
 * @brief checks if the requested method is allouwed on the current method
 * 
 * @param location_it the info of the location as provided by the config file
 * @param client_fd file descriptor of the client
 * @param chunked flag if the response is chunked
 * @param event the events of the client
 * @param method the method the client used
 * @return 1 when method is allouwed,
 * @return 0 when method is not allouwed, and response is send to the client,
 * @return -1  when method is not allouwed, and response to the client failed
 */
int Server::checkAllowedMethods(std::vector<Location>::iterator& location_it, int client_fd, bool& chunked, epoll_event* event, std::string& method)
{
    std::vector<std::string>::const_iterator method_it = location_it->getAllowedMethods().begin();
    std::vector<std::string>::const_iterator method_ite = location_it->getAllowedMethods().end();
    while (method_it != method_ite)
    {
        if (method_it->empty())
        {
            std::cerr << "handle respone request buffer not empty";
            return setupResponse(client_fd, "500 Internal Server Error", chunked, event, error_pages_, 500);
            }
        if (method_it->compare(method) == 0)
            break;
        ++method_it;
    }
    if (method_it == method_ite)
    {
        if (setupResponse(client_fd, "400 Bad Request", chunked, event, error_pages_, 400) == -1)
        {
            if (setupResponse(client_fd, "500 Internal Server Error", chunked, event, error_pages_, 500) == -1)
                close(client_fd);
            return -1;
        }
        return 0;
    }
    return 1;
}

/**
 * @brief checks if the file request by the client exist and if we have permissions to open it
 * 
 * @param file_path the path to the file we want to check
 * @param client_fd the file descriptor of the client
 * @param chunked flag if the response is chunked
 * @param event the events of the client
 * @param location_it for fallback check if for the file
 * @return 1 when response is send to the client,
 * @return -1 on send error
 */
int Server::checkFile(std::string& file_path, int client_fd, bool& chunked, epoll_event* event, std::vector<Location>::iterator& location_it)
{
    if (fileExists(file_path))
    {
        if (filePermission(file_path))
        {
            std::cout << "file_path: " << file_path << std::endl;
            return fileResponseSetup(client_fd, "200 OK", chunked, event, 200, file_path);
        }
        else
            return fileResponseSetup(client_fd, "403 Forbidden", chunked, event, 403);
    }
    else if (location_it->getAutoindex())
        return 0;
    else if (fileExists(root_path_ + location_it->getIndex()))
    {
        if (filePermission(root_path_ + location_it->getIndex()))
            return fileResponseSetup(client_fd, "200 OK", chunked, event, 200, root_path_ + location_it->getIndex());
        else
            return fileResponseSetup(client_fd, "403 Forbidden", chunked, event, 403);
    }
    else
    {
        std::cerr << "file_path: \"" << file_path << "\" not found\n";
        return fileResponseSetup(client_fd, "404 Not Found", chunked, event, 404);
    }
}

/**
 * @brief sets up the response for file persittion function
 * 
 * @param client_fd the file descriptor of the client
 * @param code_string the code text needed in the header
 * @param chunked flag if the response is chunked
 * @param event the events of the client
 * @param code what response code is used
 * @param location optional location of file
 * @return 1 when response is send,
 * @return -1 when send fails
 */
int Server::fileResponseSetup(int client_fd, std::string code_string, bool& chunked, epoll_event* event, uint code, std::string location)
{
    if (code == 200)
    {
        if (setupResponse(client_fd, code_string, chunked, event, error_pages_, code, location))
        {
            if (setupResponse(client_fd, "500 Internal Server Error", chunked, event, error_pages_, 500) == -1)
                close(client_fd);
            return -1;
        }
    }
    else
    {
        if (setupResponse(client_fd, code_string, chunked, event, error_pages_, code) == -1)
        {
            if (setupResponse(client_fd, "500 Internal Server Error", chunked, event, error_pages_, 500) == -1)
                close(client_fd);
            return -1;
        }
    }
    return 1;
}

/**
 * @brief setups the response the client will recieve
 * 
 * @param client_fd file descriptor if the client
 * @param status the code text needed in the header
 * @param chunked if the response is chunked
 * @param event the events of the client
 * @param error_pages the erorror pages know by the server
 * @param number what code is being send 
 * @param location optinal location of the file being send
 * @return 0 when done,
 * @return -1 on send response error
 */
int Server::setupResponse(int client_fd, std::string status, bool& chunked, epoll_event* event, std::map<uint, std::string>& error_pages, uint number, std::string location)
{
    if (number == 200)
    {
        if (sendResponse(client_fd, status, location, chunked) == -1)
            return -1;
        doEpollCtl(EPOLL_CTL_DEL, client_fd, event);
        close(client_fd);
    }
    else
    {  
        std::map<uint, std::string>::iterator error_page = error_pages.find(number);
        if (error_page == error_pages.end())
        {
            std::string fall_back = "example/errorPages/";
            fall_back.append(std::to_string(number));
            fall_back.append(".html");
            if (sendResponse(client_fd, status, fall_back, chunked) == -1)
                return -1;
            doEpollCtl(EPOLL_CTL_DEL, client_fd, event);
            close(client_fd);
        }
        else
        {
            location.insert(0UL, config_.getRoot());
            if (sendResponse(client_fd, status, location + error_page->second, chunked) == -1)
                return -1;
            doEpollCtl(EPOLL_CTL_DEL, client_fd, event);
            close(client_fd);
        }
    }
    return 0;
}

/**
 * @brief makes the response and sends it
 * 
 * @param client_fd the file descriptor of the client who gets the response
 * @param status the status of the response
 * @param file_location the file location of the response
 * @param chunked if the response needs to be chunked
 * @return 0 when response has been send,
 * @return -1 on error
 */

int Server::sendResponse(int client_fd, const std::string& status, const std::string& file_location, bool& chunked, bool d_list)
{
    bool content = false;
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Connection: close\r\n";

    if (d_list)
    {
        response << "Content-Type" << getContentType("x.html") << "\r\n";
        response << "Content-Length: " << file_location.size() << "\r\n\r\n";
        response << file_location;
        if (send(client_fd, response.str().c_str(), response.str().size(), 0) == -1)
            return -1;
        return 0;
    }

    response << "Content-Type: " << getContentType(file_location) << "\r\n";
    std::ifstream file_stream(file_location, std::ios::binary);
    if (!file_stream.is_open())
    {
        std::cerr << "files_stream open: " << file_location << std::endl;
        return -1;
    }


    if (chunked)
    {
        response << "Transfer-Encoding: chunked\r\n\r\n";
        if (send(client_fd, response.str().c_str(), response.str().size(), 0) < 0)
            return -1;
        if (sendChunkedResponse(client_fd, file_stream) == -1)
            return -1;
        chunked = false;
    }
    else
    {
        if (file_stream.good())
        {
            content = true;
            file_stream.seekg(0, std::ios::end);
            std::streamsize file_size = file_stream.tellg();
            file_stream.seekg(0, std::ios::beg);
            response << "Content-Length: " << file_size << "\r\n\r\n";
        }
        else
            response << "Content-Length: 0\r\n\r\n";
        if (send(client_fd, response.str().c_str(), response.str().size(), 0) < 0)
            return -1;
        if (content)
        {
            if (sendFile(client_fd, file_stream) == -1)
            	return -1;
        }
    }
    return 0;
}

/**
 * @brief sends back a chunked response
 * 
 * @param client_Fd the file descriptor of the client
 * @param file_stream the file stream of the response needed to send
 * @return 0 when all chunks have been send,
 * @return -1 on send error
 */
int Server::sendChunkedResponse(int client_Fd, std::ifstream& file_stream)
{
    char buffer[BUFFER_SIZE];
    while(file_stream.read(buffer, BUFFER_SIZE) || file_stream.gcount() > 0)
    {
        std::ostringstream chunk;
        chunk << std::hex << file_stream.gcount() << "\r\n"; // chunk size in hex
        if(send(client_Fd, chunk.str().c_str(), chunk.str().size(), 0) < 0)
            return -1;
        if(send(client_Fd, buffer, file_stream.gcount(), 0) < 0)
            return -1;
        if (send(client_Fd, "\r\n", 2, 0) < 0)
            return -1;
    }
    if (send(client_Fd, "0\r\n\r\n", 5, 0) < 0) // Last chunk (size 0) indicates end of transfer
        return -1;
    return 0;
}

/**
 * @brief sends back non chunked response
 * 
 * @param client_fd the file descriptor of the client
 * @param file_stream the file stream of the file being send 
 * @return 0 when,
 * @return -1 on send error
 */
int Server::sendFile(int client_fd, std::ifstream& file_stream)
{
    char buffer[BUFFER_SIZE];
    if (file_stream.bad())
        return -1;
    while (file_stream.read(buffer, BUFFER_SIZE) || file_stream.gcount() > 0)
    {
        if (send(client_fd, buffer, file_stream.gcount(), MSG_NOSIGNAL) < 0)
            return -1;
    }
	return 0;
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