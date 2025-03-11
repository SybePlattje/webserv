#ifndef SERVER_RESPONSE_HANDLER_HPP
# define SERVER_RESPONSE_HANDLER_HPP

# include "server/ServerRequestHandler.hpp"
# include "server/ServerResponseValidator.hpp"
# include "../Config.hpp"
# include <sys/epoll.h>
# include <vector>

# define STANDARD_LOG_FILE "log.log"
# define STANDARD_ERROR_LOG_FILE "error.log"

enum e_server_request_return
{
    SRH_OK,
    SRH_INCORRECT_HTTP_VERSION,
    SRH_OPEN_DIR_FAILED,
    SRH_SEND_ERROR,
    SRH_FSTREAM_ERROR,
};

class ServerResponseHandler
{
    public:
        ServerResponseHandler(const std::vector<std::shared_ptr<Location>>& locations, const std::string& root, const std::map<uint16_t, std::string>& error_map);
        ~ServerResponseHandler();
        e_server_request_return handleResponse(int client_fd, s_client_data client_data, std::vector<std::shared_ptr<Location>> locations);
        e_server_request_return setupResponse(int client_fd, std::string status_text, uint32_t code, s_client_data data, std::string location = "");
        void handleCoutErrOutput(int fd);
        void setStdoutPipe(int stdout_pipe[]);
    private:
        ServerResponseValidator SRV_;
        const std::map<uint16_t, std::string>& error_pages_;
        int stdout_pipe_[2];

        e_server_request_return handleReturns(int client_fd, e_responeValReturn nr, s_client_data data);
        e_server_request_return buildDirectoryResponse(const std::string& path, std::string& body);
        e_server_request_return sendResponse(int client_fd, const std::string& status, const std::string& file_location, s_client_data& data, bool d_list = false);
        std::string getContentType(const std::string& file_path);
        e_server_request_return sendChunkedResponse(int client_fd, std::ifstream& file_stream);
        e_server_request_return sendFile(int client_fd, std::ifstream& file_stream);
        std::vector<std::string> sourceChunker(std::string& source);
        void logMsg(const char* msg, int fd);
};

#endif