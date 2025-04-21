#ifndef SERVER_HPP
# define SERVER_HPP

# define MAX_EVENTS 1024
# define TIMEOUT_MS 20000 // 20 seconds
# define EPOLL_WAIT_TIME 10000 // 10 seconds

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

struct configInfo
{
    configInfo(std::shared_ptr<Config>& conf);
    ServerRequestHandler requestHandler_;
    ServerResponseHandler responseHandler_;
    std::shared_ptr<Config> config_;
    std::string root_folder_;
    std::string main_index_;
    std::vector<std::shared_ptr<Location>> locations_;
    std::map<uint16_t, std::string> error_pages_;
    int server_fd_;
    std::string server_name_;
    uint16_t port_;
};

class Server
{
    public:
        Server(std::vector<std::shared_ptr<Config>>& config);
        ~Server();
        int setupEpoll();
        int serverLoop();
    protected:
    private:
        std::vector<configInfo> config_info_;
        size_t conf_size_;
        ServerValidator validator_;
        int epoll_fd_;
        int stdout_pipe_[2];
        int stderr_pipe_[2];
        std::unordered_map<int, int> client_timers_;


        int createServerSocket(std::string& server_name, uint16_t port, int& server_fd);
        sockaddr_in setServerAddr(std::string& server_name, uint16_t port);
        int bindServerSocket(sockaddr_in& server_addr, int server_fd);
        int listenServer(int server_fd);
        void setNonBlocking(int fd);
        int doEpollCtl(int mode, int fd, epoll_event* event);
        int setupPipe();
        int putCoutCerrInEpoll();
        int listenLoop();
        int checkEvents(epoll_event event);
        int setupConnection(int server_fd, configInfo& config);
        int setTimer(int client_fd);
        int checkForTimeout(int fd, epoll_event& event);
        int handleReadEvents(int fd, epoll_event& event);
        std::string epollEventToString(uint32_t events);
        std::string getFdType(int fd);
};

#endif