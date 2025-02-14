#ifndef SERVER_HPP
# define SERVER_HPP

# define MAX_EVENTS 10
# define BUFFER_SIZE 1024
# define TIMEOUT_MS 10000 // 10 seconds

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

class Server
{
	public:
		Server(Config& config);
		~Server();
		int	setupEpoll();
	private:
		std::string 							request_buffer_;
		std::unordered_map<int, e_ErrorInfo> 	error_map_;
		int 									server_fd_;
		int										epoll_fd_;
		Config&									config_;

		int				createServerSocket(uint port, const std::string& server_ip);
		sockaddr_in		setServerAddr(const std::string& server_ip, uint port);
		int 			bindServerSocket(sockaddr_in& server_addr);
		int 			listenServer();
		int 			checkEvent(epoll_event events, std::string& method, std::string& source, std::string& http_version, bool& chunked);
		void			setNonBlocking(int fd);
		bool			fileExists(const std::string& path);
		void			sendResponse(int client_fd, const std::string& status, const std::string& content, bool& chunked);
		void			handleResponse(int client_fd, epoll_event* event, std::string& method, std::string& source, std::string& http_version, bool& chunked);
		int				handleClient(int client_fd, bool& chunked);
		void			closeFd(int fd_a = -1, int fd_b = -1, int fd_c = -1);
		int				listenLoop();
		int				setupConnection();
		int				checkErrno(int err, int fd = -1, epoll_event events = {});
		void			fillErrorMap();
		int				doEpollCtl(int mode, int client_fd, epoll_event* event);
		bool			filePermission(const std::string& path);
		std::string 	readRequest(int client_fd, std::string& method, std::string& source, std::string& http_version, bool& chunked);
		std::string		handleContentLength(size_t content_length_pos, std::string& headers, std::string& request_buffer, size_t body_start, int client_fd, char buffer[]);
		std::string		handleChunkedRequest(size_t body_start, std::string& request_buffer, int clientFd, char buffer[]);
		int 			useRevc(int client_fd, char buffer[], std::string& requestBuffer);
		int				doClientModification(int client_fd, epoll_event* event, const std::string& status, const std::string& location, bool& chunked);
		int 			doClientDelete(int client_fd, const std::string& status, const std::string location, epoll_event* event, bool& chunked);
		void			setMethodSourceHttpVerion(std::string& method, std::string& source, std::string& http_version, std::string& request_buffer);
		void			sendChunkedResponse(int client_fd, std::ifstream& file_stream);
		void			sendFile(int client_fd, std::ifstream& file_stream);
		std::string		getContentType(const std::string& file_path);
		void setupResponse(int client_fd, std::string status, bool& chunked, epoll_event* event, std::map<uint, std::string>& error_pages, uint error_number, std::string location = "");
};

#endif