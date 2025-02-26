#ifndef SERVER_HPP
# define SERVER_HPP

# define MAX_EVENTS 10
# define BUFFER_SIZE 1024
# define TIMEOUT_MS 10000 // 10 seconds
# define STANDARD_LOG_FILE "log.log"
# define STANDARD_ERROR_LOG_FILE "error.log"

# include <string>
# include <sys/epoll.h>
# include <netinet/in.h>
# include <unordered_map>
# include "Config.hpp"

struct e_ErrorInfo 
{
	std::string message;
	int return_value;
};

enum e_RequestKeys
{
	REQUEST_TYPE,
	REQUEST_HEADER,
	REQUEST_BODY,
};

class Server
{
	public:
		Server(Config& config);
		~Server();
		int	setupEpoll();
	private:
		std::map<e_RequestKeys, std::string>	request_;
		std::string 							request_buffer_;
		std::unordered_map<int, e_ErrorInfo> 	error_map_;
		int 									server_fd_;
		int										epoll_fd_;
		Config&									config_;
		int										stdout_pipe_[2];
		int										stderr_pipe_[2];
		std::string								root_path_;
		std::string								main_index_;
		std::vector<Location>					locations_;
		std::map<uint, std::string>				error_pages_;


		int createServerSocket(uint port, const std::string& server_ip);
		sockaddr_in setServerAddr(const std::string& server_ip, uint port);
		int bindServerSocket(sockaddr_in& server_addr);
		int listenServer();
		int checkEvent(epoll_event events, std::string& method, std::string& source, std::string& http_version, bool& chunked);
		void setNonBlocking(int fd);
		bool fileExists(const std::string& path);
		int sendResponse(int client_fd, const std::string& status, const std::string& content, bool& chunked, bool d_list = false);
		int handleResponse(int client_fd, epoll_event* event, std::string& method, std::string& source, std::string& http_version, bool& chunked);
		int handleClient(int client_fd, bool& chunked);
		void closeFd(int fd_a = -1, int fd_b = -1, int fd_c = -1);
		int listenLoop();
		int setupConnection();
		int checkErrno(int err, int fd = -1, epoll_event events = {});
		void fillErrorMap();
		int doEpollCtl(int mode, int client_fd, epoll_event* event);
		bool filePermission(const std::string& path);
		std::string readRequest(int client_fd, std::string& method, std::string& source, std::string& http_version, bool& chunked, bool& max_size);
		std::string handleContentLength(size_t content_length_pos, std::string& headers, std::string& request_buffer, size_t body_start, int client_fd, char buffer[], bool& max_size);
		std::string handleChunkedRequest(size_t body_start, std::string& request_buffer, int clientFd, char buffer[]);
		int useRevc(int client_fd, char buffer[], std::string& requestBuffer);
		int doClientModification(int client_fd, epoll_event* event, const std::string& status, const std::string& location, bool& chunked);
		int doClientDelete(int client_fd, const std::string& status, const std::string location, epoll_event* event, bool& chunked);
		void setMethodSourceHttpVerion(std::string& method, std::string& source, std::string& http_version, std::string& request_buffer);
		int sendChunkedResponse(int client_fd, std::ifstream& file_stream);
		int sendFile(int client_fd, std::ifstream& file_stream);
		std::string getContentType(const std::string& file_path);
		int setupResponse(int client_fd, std::string status, bool& chunked, epoll_event* event, std::map<uint, std::string>& error_pages, uint error_number, std::string location = "");
		void logMsg(const char* msg, int fd);
		int setupPipe();
		int putCoutCerrInEpoll();
		std::string handleCoutErrOutput(int client_fd);
		int setContentTypeRequest(std::string request_buffer, size_t header_end);
		int checkHTTPVersion(std::string& http_version, int client_Fd, bool& chunked, epoll_event* event);
		int checkLocations(std::vector<std::string>& token_location, std::string& file_path, int client_fd, bool& chunked, epoll_event* event, std::vector<Location>::iterator& location_it);
		int checkAllowedMethods(std::vector<Location>::iterator& location_it, int client_fd, bool& chunked, epoll_event* event, std::string& method);
		int checkFile(std::string& file_path, int client_fd, bool& chunked, epoll_event* event, std::vector<Location>::iterator& location_it);
		int fileResponseSetup(int client_fd, std::string code_string, bool& chunked, epoll_event* event, uint code, std::string location = "");
        std::vector<std::string> sourceChunker(std::string source);
        int checkAutoIndexing(std::vector<Location>::iterator location_it, int client_fd, bool& chunked, epoll_event* event);
        bool isDirectory(std::string& path);
        int buildDirectoryResponse(const std::string& path, std::string& body);
        std::string readHeader(std::string request_buffer, size_t header_end, std::string& method, std::string& source, std::string http_version, bool& chunked, int client_fd, char buffer[], bool& max_size);
        void setPosibleLocations(size_t token_size, std::vector<std::string>& token_location, std::map<size_t, std::string>& found_location);
};

#endif