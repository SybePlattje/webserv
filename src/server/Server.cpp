#include "Server.hpp"
#include <sys/socket.h>
#include <fcntl.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <sys/timerfd.h>

Server::Server(std::unique_ptr<Config>& config) : validator_(), requestHandler_(config->getClientMaxBodySize()), responseHandler_(config.get()->getLocations(), config.get()->getRoot(), config.get()->getErrorPages()), config_(std::move(config))
{
    std::string server_name = config_->getServerName();
    if (server_name == "localhost")
        server_name = "127.0.0.1";
    if (createServerSocket(server_name, config_->getPort()) != 0)
    {
        std::cerr << "create server socket error\n";
        throw std::runtime_error("failed to setup server socket");
    }
    root_folder_ = config_->getRoot();
    main_index_ = config_->getIndex();
    locations_ = config_->getLocations();
    error_pages_ = config_->getErrorPages();
}

Server::~Server() {};

/**
 * @brief Creates the epoll that will hold all events to listen to. 
 * Sets up the server socket, and adds the needed fd to the epoll
 * 
 * @return 0 when done,
 * @return -1 on error,
 * @return -2 on cirtical error
 */
int Server::setupEpoll()
{
    epoll_fd_ = epoll_create(MAX_EVENTS);
    if (epoll_fd_ == -1)
    {
        std::cerr << "epoll_create error\n";
        int nr = validator_.checkErrno(errno);
        close(server_fd_);
        return nr;
    }
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = server_fd_;

    int nr = doEpollCtl(EPOLL_CTL_ADD, server_fd_, &event);
    if (nr != 0)
    {
        std::cerr << "adding serverFd failed\n";
        close(epoll_fd_);
        close(server_fd_);
        return nr;
    }

    // if (setupPipe() != 0)
    // {
    //     close(epoll_fd_);
    //     close(server_fd_);
    //     return -1;
    // }
    // responseHandler_.setStdoutPipe(stdout_pipe_);
    // requestHandler_.setStdoutPipe(stdout_pipe_);
    // requestHandler_.setStderrPipe(stderr_pipe_);

    // nr = putCoutCerrInEpoll();
    // if (nr < 0)
    // {
    //     close(stdout_pipe_[0]);
    //     close(stderr_pipe_[0]);
    //     close(epoll_fd_);
    //     close(server_fd_);
    //     return nr;
    // }

    nr = listenLoop();
    if (nr < 0)
    {
        // close(stdout_pipe_[0]);
        // close(stderr_pipe_[0]);
        close(epoll_fd_);
        close(server_fd_);
        return nr;
    }
    // close(stdout_pipe_[0]);
    // close(stderr_pipe_[0]);
    return 0;
}

// private functions

/**
 * @brief makes the server socket and sets it up to the given port,
 * It also makes the server fd non blocking
 * 
 * @param server_name the name of the server
 * @param port on what port the server socket will listen
 * @return 0 when done,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::createServerSocket(std::string& server_name, uint16_t port)
{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1)
        return 1;
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in server_addr = setServerAddr(server_name, port);
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
 * @brief sets the server address information for the server.
 * 
 * @param server_name the name of the server
 * @param port the port the server is bound to 
 * @return the server address information
 */
sockaddr_in Server::setServerAddr(std::string& server_name, uint16_t port)
{
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_name.c_str());
    server_addr.sin_port = htons(port);
    return server_addr;
}

/**
 * @brief binds the server socket and the server address info together
 * 
 * @param server_addr the server address information
 * @return 0 when done,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::bindServerSocket(sockaddr_in& server_addr)
{
    if (bind(server_fd_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "bind error\n";
        int nr = validator_.checkErrno(errno);
        close(server_fd_);
        return nr;
    }
    return 0;
}

/**
 * @brief makes it so the server listen to inkoming requests
 * 
 * @return 0 when done,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::listenServer()
{
    if (listen(server_fd_, SOMAXCONN) < 0)
    {
        std::cerr << "listen errro\n";
        int nr = validator_.checkErrno(errno);
        close(server_fd_);
        return nr;
    }
    return 0;
}

/**
 * @brief sets the file descriptor to be non blocking
 * 
 * @param fd the file descriptor to be set non blocking
 */
void Server::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, fd);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief does a epoll ection on the epoll fd to the given fd.
 * It also tries to save some errors internaly, 
 * like trying to add a fd that is already in the epoll
 * or modifying a fd that doesn't exists in the epoll
 * 
 * @param mode what action is taken
 * @param fd the file descriptor of who the action is taken on
 * @param event the new event struct for the fd
 * @return 0 when done,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::doEpollCtl(int mode, int fd, epoll_event* event)
{
    if (epoll_ctl(epoll_fd_, mode, fd, event) == -1)
    {
        std::cerr << "first round epoll_ctl error\n";
        int nr = validator_.checkErrno(errno);
        if (nr == 1)
        {
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, event) == -1)
            {
                std::cerr << "epoll mod error\n";
                nr = validator_.checkErrno(errno);
                return nr;
            }
        }
        else if (nr == 2)
        {
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, event) == -1)
            {
                std::cerr << "epoll add error\n";
                nr = validator_.checkErrno(errno);
                return nr;
            }
        }
        else if (nr < 0)
            return nr;
    }
    return 0;
}
/**
 * @brief makes pipes for the standard output and standard error
 * and makes them non blocking
 * 
 * @return 0 when done,
 * @return -1 if pipe creation failed
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

    close(stdout_pipe_[1]);
    close(stderr_pipe_[1]);

    std::cout.setf(std::ios::unitbuf);

    return 0;
}

/**
 * @brief adds the pipes that represend the standard output and standard error to the epoll
 * 
 * @return 0 when done
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
 * @brief the main loop that listens to the events that need to be handled
 * 
 * @return 0 when done,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::listenLoop()
{
    epoll_event events[MAX_EVENTS];
    while (true)
    {
        int event_count = epoll_wait(epoll_fd_, events, MAX_EVENTS, TIMEOUT_MS);
        for (int i = 0; i < event_count; ++i)
        {
            int nr = checkEvents(events[i]);
            if (nr < 0)
                return nr;
        }
    }
    close(epoll_fd_);
    close(server_fd_);
    return 0;
}

/**
 * @brief checks what action to take on the based on the fd of the event.
 * If it's  a new fd then a new connection is being made.
 * If the events hold the status of EPOLLIN than first it will check if the fd is a timeout fd
 * if it isn't than a read event needs to be handeled.
 * If the events hold the status of EPOLLOUT than first it will check if the fd is a timeout fd 
 * if it isn't than a write events needs to be handeled.
 * 
 * @param event the event with the fd and the events needed for handeling
 * @return 0 when done,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::checkEvents(epoll_event event)
{
    int fd = event.data.fd;
    if (fd == server_fd_) // new connection
    {
        if (setupConnection() == -2)
        {
            close(epoll_fd_);
            close(server_fd_);
            return -2;
        }
        return 0;
    }
    else if (event.events & EPOLLIN) // read event
    {
        std::string request_buffer;
        e_reponses function_response = requestHandler_.readRequest(fd, request_buffer);
        if (function_response != E_ROK)
        {
            if (function_response == READ_HEADER_BODY_TOO_LARGE)
            {
                int return_value = responseHandler_.setupResponse(fd, 413, requestHandler_.getRequest(fd));
                requestHandler_.removeNodeFromRequest(fd);
                return return_value;
            }
            else if (function_response == HANDLE_COUT_CERR_OUTPUT)
            {
                responseHandler_.handleCoutErrOutput(fd);
                return 0;
            }
            responseHandler_.setupResponse(fd, 400, requestHandler_.getRequest(fd));
            requestHandler_.removeNodeFromRequest(fd);
            return -2;
        }
        function_response = requestHandler_.handleClient(request_buffer, event);
        if (function_response == MODIFY_CLIENT_WRITE)
        {
            if (doEpollCtl(EPOLL_CTL_MOD, fd, &event) != 0)
            {
                requestHandler_.removeNodeFromRequest(fd);
                doEpollCtl(EPOLL_CTL_DEL, fd, &event);
                return -1;
            }
            // requestHandler_.removeNodeFromRequest(fd);
            return 0;
        }
        return -2;
    }
    else if (event.events & EPOLLOUT) // write 
    {
        e_server_request_return nr = responseHandler_.handleResponse(fd, requestHandler_.getRequest(fd), config_->getLocations());
        // TODO remove if statement for eval
        if (nr != SRH_DO_TIMEOUT)
        {
            doEpollCtl(EPOLL_CTL_DEL, fd, &event);
            doEpollCtl(EPOLL_CTL_DEL, client_timers_[fd], nullptr);
            close(fd);
            close(client_timers_[fd]);
            requestHandler_.removeNodeFromRequest(fd);
            client_timers_.erase(fd);
            if (nr != SRH_OK)
                return -2;
            return 0;
        }
        return 0;
    }
    return -2;
}

/**
 * @brief when a new client connecting we set the socket and the event represending the client up
 * 
 * @return 0 when dome,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::setupConnection()
{
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    int client_fd = accept(server_fd_, (sockaddr*)&clientAddr, &clientLen);
    if (client_fd != -1)
    {
        setNonBlocking(client_fd);
        epoll_event client_event{};
        client_event.events = EPOLLIN;
        client_event.data.fd = client_fd;
        int nr = doEpollCtl(EPOLL_CTL_ADD, client_fd, &client_event);
        if (nr != 0)
        {
            std::cerr << "setup connecton new client to epoll failed\n";
            return nr;
        }
        nr = setTimer(client_fd);
        if (nr != 0)
            return nr;
        return 0;
    }
    else
    {
        std::cerr << "accept error\n";
        return validator_.checkErrno(errno);
    }
}

/**
 * @brief sets a timeout timer for the client so we dont have hanging connections
 * 
 * @param client_fd the client fd the timer is for
 * @return 0 when done,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::setTimer(int client_fd)
{
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1)
    {
        std::cerr << "failed to create timerfd\n";
        return -1;
    }

    itimerspec timeout{};
    timeout.it_value.tv_sec = TIMEOUT_MS / 1000; // timeout in seconds
    timeout.it_value.tv_nsec = (TIMEOUT_MS % 1000) * 1000000; // timeout in nanoseconds
    timeout.it_interval.tv_sec = 0; //timer needs to go once, no periodic triggering
    timeout.it_interval.tv_nsec = 0;

    timerfd_settime(timer_fd, 0, &timeout, nullptr);

    epoll_event timer_event{};
    timer_event.data.fd = timer_fd;
    timer_event.events = EPOLLIN;
    int nr = doEpollCtl(EPOLL_CTL_ADD, timer_fd, &timer_event);
    if (nr != 0)
    {
        std::cerr << "add timer fd to epoll failed\n";
        return nr;
    }
    client_timers_[client_fd] = timer_fd;
    return 0;
}

/**
 * @brief checks if the fd is a timer fd, if it is that means a timeout has happend
 * 
 * @param fd the file descriptor of the timer
 * @param event the epoll event from the fd
 * @return 1 when the fd is not a timer fd,
 * @return 0 when timeout response is send,
 * @return -1 on error,
 * @return -2 on critical error
 */
int Server::checkForTimeout(int fd, epoll_event& event)
{
    for (const std::pair<int, int> timer : client_timers_)
    {
        if (timer.second == fd)
        {
            int client_fd = timer.first;

            uint64_t expirations;
            ssize_t bytes_read = read(fd, &expirations, sizeof(expirations));
            if (bytes_read < 0)
            {
                std::cerr << "Timeout read failed\n";
                return -2;
            }
            // do client timeout
            // TODO add cgi killer
            int nr = responseHandler_.setupResponse(client_fd, 408, requestHandler_.getRequest(fd));
            std::cout << "client timeout for " << client_fd << " reached\n";
            doEpollCtl(EPOLL_CTL_DEL, client_fd, &event);
            doEpollCtl(EPOLL_CTL_DEL, fd, nullptr);
            close(client_fd);
            close(fd);
            client_timers_.erase(client_fd);
            requestHandler_.removeNodeFromRequest(client_fd);
            if (nr == SRH_OK)
                return 0;
            return nr;
        }
    }
    return 1;
}

/**
 * @brief reads into the request from the client and stores it for later handling
 * 
 * @param fd the client file descriptor
 * @param event the epoll event from the client
 * @return 0 when request is stored,
 * @return -1 on eror,
 * @return -2 on critical error
 */
int Server::handleReadEvents(int fd, epoll_event& event)
{
    std::string request_buffer;
    e_reponses function_response = requestHandler_.readRequest(fd, request_buffer);
    if (function_response != E_ROK)
    {
        if (function_response == READ_HEADER_BODY_TOO_LARGE)
        {
            int return_value = responseHandler_.setupResponse(fd, 413, requestHandler_.getRequest(fd));
            requestHandler_.removeNodeFromRequest(fd);
            return return_value;
        }
        else if (function_response == HANDLE_COUT_CERR_OUTPUT)
        {
            responseHandler_.handleCoutErrOutput(fd);
            return 0;
        }
        responseHandler_.setupResponse(fd, 400, requestHandler_.getRequest(fd));
        requestHandler_.removeNodeFromRequest(fd);
        return -1;
    }
    function_response = requestHandler_.handleClient(request_buffer, event);
    if (function_response == MODIFY_CLIENT_WRITE)
    {
        if (doEpollCtl(EPOLL_CTL_MOD, fd, &event) != 0)
        {
            std::cerr << "modify client in main loop failed\n";
            requestHandler_.removeNodeFromRequest(fd);
            doEpollCtl(EPOLL_CTL_DEL, fd, &event);
            return -1;
        }
        // requestHandler_.removeNodeFromRequest(fd);
        return 0;
    }
    return -2;
}