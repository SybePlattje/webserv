#ifndef SERVER_RESPONSE_VALIDATOR_HPP
# define SERVER_RESPONSE_VALIDATOR_HPP

# include <string>
# include <sys/epoll.h>
# include <vector>
# include "../config/Location.hpp"
# include "../Config.hpp"
# include "ServerRequestHandler.hpp"

enum e_responeValReturn
{
    RVR_OK,
    RVR_RETURN,
    RVR_ROOT,
    RVR_NOT_FOUND,
    RVR_BUFFER_NOT_EMPTY,
    RVR_METHOD_NOT_ALLOWED,
    RVR_NO_FILE_PERMISSION,
    RVR_AUTO_INDEX_ON,
    RVR_FOUND_AT_ROOT,
    RVR_SHOW_DIRECTORY,
    RVR_DIR_FAILED,
    RVR_IS_REGEX,
};

class ServerResponseValidator
{
    public:
        ServerResponseValidator(const std::vector<std::shared_ptr<Location>>& locations, const std::string& root);
        ~ServerResponseValidator();
        bool checkHTTPVersion(std::string& http_version);
        e_responeValReturn checkLocations(std::vector<std::string>& token_location, std::string& file_path, std::vector<std::shared_ptr<Location>>::const_iterator& location_it, s_client_data& client_data);
        e_responeValReturn checkAllowedMethods(std::vector<std::shared_ptr<Location>>::const_iterator& location_it, std::string& method);
        e_responeValReturn checkFile(std::string& file_path, std::vector<std::shared_ptr<Location>>::const_iterator& location_it);
        e_responeValReturn checkAutoIndexing(std::vector<std::shared_ptr<Location>>::const_iterator& location_it);
        bool isDirectory(std::string& path);
        bool filePermission(const std::string& path);
        const std::string& getRoot() const;
        bool fileExists(const std::string& path);
    private:
        const std::vector<std::shared_ptr<Location>>& locations_;
        const std::string& root_;

        void setPossibleLocation(size_t token_size, std::vector<std::string>& token_location, std::map<size_t, std::shared_ptr<Location>>& found_location);
        void setPossibleRegexLocation(std::map<size_t, std::shared_ptr<Location>>& found_location, s_client_data& client_data);
};

#endif