#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include "IConfiguration.hpp"
#include "LocationConfig.hpp"
#include <map>
#include <vector>
#include <string>

class Configuration : public IConfiguration {
public:
    // Virtual destructor
    virtual ~Configuration();

    // Implementation of IConfiguration interface
    uint16_t getPort() const override { return port_; }
    const std::string& getServerName() const override { return server_name_; }
    const std::string& getRoot() const override { return root_; }
    const std::string& getIndex() const override { return index_; }
    uint64_t getClientMaxBodySize() const override { return client_max_body_size_; }
    const std::map<uint16_t, std::string>& getErrorPages() const override { return error_pages_; }
    const std::vector<LocationConfig>& getLocations() const override { return locations_; }

private:
    friend class ConfigurationBuilder;  // Allow builder to modify private members
    
    uint16_t port_ = 80;  // Default port
    std::string server_name_;
    std::string root_;
    std::string index_;
    uint64_t client_max_body_size_ = 1024 * 1024;  // Default: 1MB
    std::map<uint16_t, std::string> error_pages_;
    std::vector<LocationConfig> locations_;
};

#endif