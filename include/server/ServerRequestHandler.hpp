#ifndef SERVER_REQUEST_HANDLER_HPP
# define SERVER_REQUEST_HANDLER_HPP

# include <unordered_map>
# include <any>
# include <string>
# include <array>
# include <sys/epoll.h>

#define BUFFER_SIZE 1024

enum e_reponses {
    E_ROK,
    MODIFY_CLIENT_WRITE,
    HANDLE_CLIENT_EMPTY,
    HANDLE_COUT_CERR_OUTPUT,
    READ_REQUEST_EMPTY,
    READ_HEADER_BODY_TOO_LARGE,
    NO_CONTENT_TYPE,
    CLIENT_REQUEST_DATA_EMPTY,
    RECV_FAILED,
};

struct s_client_data
{
    std::string request_type;
    std::string request_header;
    std::string request_body;
    std::string request_method;
    std::string request_source;
    std::string http_version;
    bool chunked = false;
};

class ServerRequestHandler
{
    public:
        ServerRequestHandler(uint64_t client_body_size);
        ~ServerRequestHandler();
        s_client_data& getRequest(int fd);
        void removeNodeFromRequest(int fd);
        e_reponses readRequest(int client_fd, std::string& request_buffer);
        e_reponses handleClient(std::string& request_buffer, epoll_event& event);
        void setStdoutPipe(int out_pipe[]);
        void setStderrPipe(int err_pipe[]);
    private:
        std::unordered_map<int, s_client_data> request_;
        uint64_t max_size_;
        int stdout_pipe_[2];
        int stderr_pipe_[2];

        e_reponses readHeader(std::string& request_buffer, size_t header_end, int client_fd, char buffer[]);
        e_reponses setContentTypeRequest(std::string& request_buffer, size_t header_end, int client_fd);
        e_reponses setMethodSourceHttpVersion(std::string& request_buffer, int client_fd);
        e_reponses handleChunkedRequest(size_t body_start, std::string& request_buffer, int client_fd, char buffer[]);
        int useRecv(int client_fd, char buffer[], std::string& request_buffer);
        e_reponses handleContentLength(size_t size, std::string& request_buffer, size_t body_start, int client_fd, char buffer[]);
};

#endif