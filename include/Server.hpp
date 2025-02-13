#ifndef HANDLEIO_HPP
# define HANDLEIO_HPP

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
	int returnValue;
};

class Server
{
	public:
		Server(std::string serverIp, int serverPort);
		~Server();
		int	setupEpoll();
	private:
		std::string 							request_buffer_;
		std::unordered_map<int, e_ErrorInfo> 	error_map_;
		int 									server_fd_;
		int										epoll_fd_;

		int				createServerSocket(int port, const std::string& serverIP);
		sockaddr_in		setServerAddr(const std::string& serverIP, int port);
		int 			bindServerSocket(sockaddr_in& serverAddr);
		int 			listenServer();
		int 			checkEvent(epoll_event events, std::string& method, std::string& source, std::string& httpVersion, bool& chunked);
		void			setNonBlocking(int fd);
		bool			fileExists(const std::string& path);
		void			sendResponse(int clientFd, const std::string& status, const std::string& content, bool& chunked);
		void			handleResponse(int clientFd, epoll_event* event, std::string& method, std::string& source, std::string& httpVersion, bool& chunked);
		int				handleClient(int clientFd, bool& chunked);
		void			closeFd(int serverFd = -1, int epollFd = -1, int clientFd = -1);
		int				listenLoop();
		int				setupConnection();
		int				checkErrno(int err, int fd = -1, epoll_event events = {});
		void			fillErrorMap();
		int				doEpollCtl(int mode, int clientFd, epoll_event* event);
		bool			filePermission(const std::string& path);
		std::string 	readRequest(int clientFd, std::string& method, std::string& source, std::string& httpVersion, bool& chunked);
		std::string		handleContentLength(size_t contentLengthPos, std::string& headers, std::string& requestBuffer, size_t bodyStart, int clientFd, char buffer[BUFFER_SIZE]);
		std::string		handleChunkedRequest(size_t bodyStart, std::string& requestBuffer, int clientFd, char buffer[]);
		int 			useRevc(int clientFd, char buffer[], std::string& requestBuffer);
		int				doClientModification(int clientFd, epoll_event* event, const std::string& status, const std::string& location, bool& chunked);
		int 			doClientDelete(int clientFd, const std::string& status, const std::string location, epoll_event* event, bool& chunked);
		void			setMethodSourceHttpVerion(std::string& method, std::string& source, std::string& httpVersion, std::string& requestBuffer);
		void			sendChunkedResponse(int clientFd, std::ifstream& fileStream);
		void			sendFile(int clientFd, std::ifstream& fileStream);
		std::string		getContentType(const std::string& filePath);
};

#endif