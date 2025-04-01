#include "server/ServerRequestHandler.hpp"
#include <functional>
#include <stdexcept>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sstream>

ServerRequestHandler::ServerRequestHandler(uint64_t client_body_size) 
{
    max_size_ = client_body_size;
    stdout_pipe_[0] = -1;
    stdout_pipe_[1] = -1;
    stderr_pipe_[0] = -1;
    stderr_pipe_[1] = -1;
};

ServerRequestHandler::~ServerRequestHandler() {};

s_client_data& ServerRequestHandler::getRequest(int fd)
{
    return request_[fd];
}

void ServerRequestHandler::removeNodeFromRequest(int fd)
{
    request_.erase(fd);
}
void ServerRequestHandler::setStdoutPipe(int stdout_pipe[])
{
    stdout_pipe_[0] = stdout_pipe[0];
    stdout_pipe_[1] = stdout_pipe[1];
}

void ServerRequestHandler::setStderrPipe(int stderr_pipe[])
{
    stderr_pipe_[0] = stderr_pipe[0];
    stderr_pipe_[1] = stderr_pipe[1];
}

/**
 * @brief reads the request comming from the client
 * 
 * @param client_fd the file descriptor of the client
 * @param request_buffer the string that will hold the entire request of the client
 * @return E_ROK when done,
 * @return NO_CONTENT_TYPE if no content type is in the header,
 * @return CLIENT_REQUEST_DATA_EMPTY if the headers doesnt have a mehtod, source or HTTPVersion,
 * @return READ_HEADER_BODY_TOO_LARGE if the content length of the body is larger than what we allow,
 * @return READ_REQUEST_EMPTY if the client sends a empty request
 */
e_reponses ServerRequestHandler::readRequest(int client_fd, std::string& request_buffer)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_recieved = 0;
    if (request_.find(client_fd) == request_.end())
    {
        s_client_data node;
        request_.insert({client_fd, node});
    }

    if (client_fd == stdout_pipe_[0] || client_fd == stderr_pipe_[0])
        return HANDLE_COUT_CERR_OUTPUT;
    while ((bytes_recieved = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        request_buffer.append(buffer, bytes_recieved);
        size_t header_end = request_buffer.find("\r\n\r\n");
        if (header_end != std::string::npos)
            return readHeader(request_buffer, header_end, client_fd, buffer);
    }
    std::cerr << "read request empty at end\n";
    return READ_REQUEST_EMPTY;
}

/**
 * @brief modifies the client to let epoll know we are done reading and are ready to write to the client fd
 * 
 * @param client_fd the file descriptor of the client
 * @param request_buffer the string holding the request from the client
 * @return MODIFY_CLIENT_WRITE when everything is good and we are ready to change the event to a write event in epoll.
 * @return HANDLE_CLIENT_EMPTY request_buffer is empty error
 */
e_reponses ServerRequestHandler::handleClient(std::string request_buffer, epoll_event& event)
{
    event.events = EPOLLOUT;
    if (!request_buffer.empty())
        return MODIFY_CLIENT_WRITE;
    else
    {
        std::cerr << "handle client request buffer is empty\n";
        return HANDLE_CLIENT_EMPTY;
    }
}

// private functions

/**
 * @brief reads the header from the client and sets the info in the reqeust map
 * 
 * @param request_buffer the string holding header info
 * @param header_end position of where the header ands
 * @param client_fd the file descriptor of the header
 * @param buffer the last bytes read
 * @return E_ROK when done,
 * @return NO_CONTENT_TYPE if no content type is in the header,
 * @return CLIENT_REQUEST_DATA_EMPTY if the headers doesnt have a mehtod, source or HTTPVersion,
 * @return READ_HEADER_BODY_TOO_LARGE if the content length of the body is larger than what we allow
 */
e_reponses ServerRequestHandler::readHeader(std::string& request_buffer, size_t header_end, int client_fd, char buffer[])
{
    std::cout << request_buffer << std::endl;

    if (setMethodSourceHttpVersion(request_buffer, client_fd) == CLIENT_REQUEST_DATA_EMPTY)
        return CLIENT_REQUEST_DATA_EMPTY;

    if (setContentTypeRequest(request_buffer, header_end, client_fd) == NO_CONTENT_TYPE)
        return NO_CONTENT_TYPE;

    std::string headers = request_buffer.substr(0, header_end);
    getRequest(client_fd).request_header = headers;
    size_t body_start = header_end + 4; // Skip \r\n\r\n

    // check if it's chunked transfer encoding
    if (headers.find("Transfer-Encoding: chunked") != std::string::npos || headers.find("TE: chunked") != std::string::npos)
        return handleChunkedRequest(body_start, request_buffer, client_fd, buffer);
    size_t content_length_body = headers.find("Content-Length: ");
    if (content_length_body != std::string::npos)
    {
        size_t start = content_length_body + 16;
        std::stringstream stream(headers.substr(start));
        uint64_t size;
        stream >> size;
        if (size > max_size_)
            return READ_HEADER_BODY_TOO_LARGE;
        return handleContentLength(size, request_buffer, body_start, client_fd, buffer);
    }
    return E_ROK;
}

/**
 * @brief checks what the content type of the request from the client is
 * 
 * @param request_buffer the string holding the request from the client
 * @param header_end the position where the header ends in the request
 * @param client_fd the file descriptor of the client
 * @return E_ROK when contnet type is found and saved
 * @return NO_CONTENT_TYPE if no content type is found in request header
 */
e_reponses ServerRequestHandler::setContentTypeRequest(std::string& request_buffer, size_t header_end, int client_fd)
{
    if (request_.at(client_fd).request_method == "GET")
        return E_ROK;
    size_t request_type_pos = request_buffer.find("Content-Type: ");
    if (request_type_pos != std::string::npos)
    {
        request_type_pos += 14; // skip past "Content-Type: "
        size_t end = request_buffer.substr(request_type_pos, header_end - request_type_pos).length();
        getRequest(client_fd).request_type = request_buffer.substr(request_type_pos, end);
        return E_ROK;
    }
    return NO_CONTENT_TYPE;
}

/**
 * @brief sets the method, source, and http_version from the request to be saved
 * 
 * @param request_buffer the string holding the client requets
 * @param client_fd the file descriptor of the client
 * @return E_ROK when done
 */
e_reponses ServerRequestHandler::setMethodSourceHttpVersion(std::string& request_buffer, int client_fd)
{
    std::istringstream stream(request_buffer);
    stream >> getRequest(client_fd).request_method >> getRequest(client_fd).request_source >> getRequest(client_fd).http_version;
    return E_ROK;
}

/**
 * @brief un chunks the chunked request for better handeling later
 * 
 * @param body_start position where the body of the request starts
 * @param request_buffer the string holding the entire request
 * @param client_fd the file descriptor of the client
 * @param buffer the buffer being used to read into the request
 * @return E_ROK when done,
 * @return RECV_FAILED when reading ussing recv() failed 
 */
e_reponses ServerRequestHandler::handleChunkedRequest(size_t body_start, std::string& request_buffer, int client_fd, char buffer[])
{
    std::string decoded_body;
    size_t pos = body_start;
    while(true)
    {
        // read chunk size
        size_t chunk_size_end = request_buffer.find("\r\n", pos);
        if (chunk_size_end == std::string::npos)
        {
            if (useRecv(client_fd, buffer, request_buffer) == -1) return RECV_FAILED;
            continue;
        }
        std::string chunk_size_hex = request_buffer.substr(pos, chunk_size_end - pos);
        int chunk_size = std::stoi(chunk_size_hex, nullptr, 16);
        pos = chunk_size_end + 2; // move past \r\n
        if (chunk_size == 0) break; // end of chunks

        //ensure the full chunk is recieved
        while (request_buffer.size() < pos + chunk_size + 2)
        {
            int recv_return = useRecv(client_fd, buffer, request_buffer);
            if (recv_return == -1) return RECV_FAILED;
            if (recv_return == 0) break;
        }

        //extract chunk data
        std::string chunk_data = request_buffer.substr(pos, chunk_size);
        decoded_body.append(chunk_data);
        pos += chunk_size + 2; // move past \r\n
    }
    getRequest(client_fd).request_body = decoded_body;
    getRequest(client_fd).chunked = true;
    return E_ROK;
}

/**
 * @brief uses recv to read with buffer in the the request from the client and stores it into request_buffer
 * 
 * @param client_fd the file descriptor from the client
 * @param buffer the buffer used to read into the request from the client
 * @param request_buffer the string holding the entire request from the client
 * @return 0 when done,
 * @return -1 if recv() fails or std::length_error happens
 */
int ServerRequestHandler::useRecv(int client_fd, char buffer[], std::string& request_buffer)
{
    ssize_t bytes_recieved = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_recieved < 0)
        return -1;
    if (bytes_recieved == 0)
        return 0;
    try
    {
        request_buffer.append(buffer, bytes_recieved);
    }
    catch(std::length_error& e)
    {
        std::cerr << "str append error\n";
        std::cerr << e.what() << "\n";
        return -1;
    }
    return 0;
}

/**
 * @brief reads into the request to read max n size to get the body of the request
 * 
 * @param size the max size we need to read
 * @param request_buffer the string holding the entire request
 * @param body_start the position where the body starts in request_buffer
 * @param client_fd the file descriptor of the client
 * @param buffer the buffer being used to read the request body from the client
 * @return E_ROK when done,
 * @return RECV_FAILED if useRecv() failed 
 */
e_reponses ServerRequestHandler::handleContentLength(size_t size, std::string& request_buffer, size_t body_start, int client_fd, char buffer[])
{
    while (request_buffer.size() < body_start + size)
    {
        int recv_return = useRecv(client_fd, buffer, request_buffer);
        if (recv_return == 0) break;
        if (recv_return == -1) return RECV_FAILED;
    }
    getRequest(client_fd).request_body = request_buffer.substr(body_start, size);
    return E_ROK;
}