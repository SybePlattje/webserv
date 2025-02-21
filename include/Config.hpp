#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <map>
#include "ConfigUtils.hpp"
#include "Location.hpp"

#define DEFAULT_CONFIG "./webserv.conf"

class Config {
public:
    Config(const char *argv);
    ~Config();
    Config(const Config& other);
    Config& operator=(const Config& other);

    // Getters
    const uint& getListen() const;
    const std::string& getServerName() const;
    const std::string& getRoot() const;
    const std::string& getIndex() const;
    const std::map<uint, std::string>& getErrorPages() const;
    const uint& getClientMaxBodySize() const;
    const std::vector<Location>& getLocations() const;
    void printConfig() const;

private:
    // Configuration values
    uint listen_;
    std::string server_name_;
    std::string root_;
    std::string index_;
    std::map<uint, std::string> error_pages_;
    uint client_max_body_size_;
    std::vector<Location> locations_;

    // Directive status flags
    bool has_listen_;
    bool has_server_name_;
    bool has_root_;
    bool has_index_;
    bool has_client_max_body_size_;

    void parseServerBlock(std::istream& stream);
    void parseLocationBlock(std::istream& stream, const std::string& path);
    
    // Validation helpers
    void validatePort(const uint port);
    void validatePath(const std::string& path);
    void validateRequiredDirectives() const;
    void checkDirectiveNotSet(const std::string& directive, bool has_directive) const;
    void validateArgs(const std::string& directive, const std::vector<std::string>& tokens, size_t expected);
};

#endif