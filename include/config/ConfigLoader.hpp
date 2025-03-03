#ifndef CONFIGLOADER_HPP
#define CONFIGLOADER_HPP

#include "Config.hpp"
#include <string>
#include <memory>

# define DEFAULT_CONFIG "./webserv.conf"

class ConfigLoader {
public:
    static std::unique_ptr<Config> load(const char* path = nullptr);

private:
    ConfigLoader(); // Prevent instantiation
    ~ConfigLoader();
};

#endif