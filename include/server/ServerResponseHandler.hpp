#ifndef SERVER_RESPONSE_HANDLER_HPP
# define SERVER_RESPONSE_HANDLER_HPP

# include "server/ServerRequestHandler.hpp"
# include "server/ServerResponseValidator.hpp"
# include "cgi/CGIHandler.hpp"
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
    SRH_CGI_ERROR,
    SRH_DO_TIMEOUT,
};

class ServerResponseHandler
{
    public:
        ServerResponseHandler(const std::vector<std::shared_ptr<Location>>& locations, const std::string& root, const std::map<uint16_t, std::string>& error_map);
        ~ServerResponseHandler();
        e_server_request_return handleResponse(int client_fd, s_client_data& client_data, std::vector<std::shared_ptr<Location>> locations);
        e_server_request_return setupResponse(int client_fd, uint16_t code, const s_client_data& data, std::string location = "");
        void handleCoutErrOutput(int fd);
        void setStdoutPipe(int stdout_pipe[]);
    private:
        ServerResponseValidator SRV_;
        const std::map<uint16_t, std::string>& error_pages_;
        int stdout_pipe_[2];
        std::map<uint16_t, std::string> status_codes_;

        e_server_request_return handleReturns(int client_fd, e_responeValReturn nr, s_client_data& data, std::vector<std::shared_ptr<Location>>::const_iterator& location_it);
        e_server_request_return buildDirectoryResponse(const std::string& path, std::string& body);
        e_server_request_return sendResponse(int client_fd, const std::string& status, const std::string& file_location, const s_client_data& data, bool d_list = false);
        std::string getContentType(const std::string& file_path);
        e_server_request_return sendChunkedResponse(int client_fd, std::ifstream& file_stream);
        e_server_request_return sendFile(int client_fd, std::ifstream& file_stream, std::streamsize size, std::ostringstream& response);
        std::vector<std::string> sourceChunker(std::string& source);
        void logMsg(const char* msg, int fd);

        /**
         * @brief Handle CGI request processing
         * @param client_fd Client socket
         * @param client_data Request data
         * @param location Location configuration
         * @param script_path Path to CGI script
         * @return SRH_OK on success, error code otherwise
         */
        e_server_request_return handleCGI(
            int client_fd,
            const s_client_data& client_data,
            const Location& location,
            const std::string& script_path);
        e_server_request_return sendRedirectResponse(int client_fd, uint16_t code, std::string& location);
        void fillStatusCodes();
        e_server_request_return removeFile(int client_fd, s_client_data& client_data);
};

#endif