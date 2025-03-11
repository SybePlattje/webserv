#ifndef SERVER_HPP
# define SERVER_HPP

# define MAX_EVENTS 10
# define TIMEOUT_MS 10000 // 10 seconds

# include "Config.hpp"
# include <string>
# include <vector>
# include "config/Location.hpp"
# include <map>
# include <cstdint>
# include "server/ServerValidator.hpp"
# include "server/ServerRequestHandler.hpp"
# include "server/ServerResponseHandler.hpp"
# include <arpa/inet.h>

class Server
{
    public:
        Server(std::unique_ptr<Config>& config);
        ~Server();
        int setupEpoll();
    protected:
    private:
        ServerValidator validator_;
        ServerRequestHandler requestHandler_;
        ServerResponseHandler responseHandler_;
        std::unique_ptr<Config> config_;
        std::string root_folder_;
        std::string main_index_;
        std::vector<std::shared_ptr<Location>> locations_;
        std::map<uint16_t, std::string> error_pages_;
        int server_fd_;
        int epoll_fd_;
        int stdout_pipe_[2];
        int stderr_pipe_[2];


        int createServerSocket(std::string& server_name, uint16_t port);
        sockaddr_in setServerAddr(std::string& server_name, uint16_t port);
        int bindServerSocket(sockaddr_in& server_addr);
        int listenServer();
        void setNonBlocking(int fd);
        int doEpollCtl(int mode, int fd, epoll_event* event);
        int setupPipe();
        int putCoutCerrInEpoll();
        int listenLoop();
        int checkEvents(epoll_event event);
        int setupConnection();
};

#endif