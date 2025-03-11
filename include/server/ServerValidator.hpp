#ifndef SERVER_VALIDATOR_HPP
# define SERVER_VALIDATOR_HPP

# include <sys/epoll.h>
# include <unordered_map>
# include <string>

struct s_ErrorInfo 
{
	std::string message;
	int return_value;
};

class ServerValidator
{
    public:
        ServerValidator();
        ~ServerValidator();
        int checkErrno(int err);
    private:
         std::unordered_map<int, s_ErrorInfo> error_map_;

        void fillErrorMap();
        const std::unordered_map<int, s_ErrorInfo>& getErrorMap() const;
};


#endif