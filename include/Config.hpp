#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <memory>

// Forward declaration
class Location;

class Config {
public:
    Config() = default;
    ~Config() = default;

    // Getters
    uint16_t getPort() const { return port_; }
    const std::string& getServerName() const { return server_name_; }
    const std::string& getRoot() const { return root_; }
    const std::string& getIndex() const { return index_; }
    uint64_t getClientMaxBodySize() const { return client_max_body_size_; }
    const std::map<uint16_t, std::string>& getErrorPages() const { return error_pages_; }
    const std::vector<std::shared_ptr<Location>>& getLocations() const { return locations_; }

private:
    friend class ConfigBuilder; // Only builder can modify configuration

    uint16_t port_ = 9999;  // Default port
    std::string server_name_;
    std::string root_ = "/";  // Default root
    std::string index_ = "index.html";  // Default index
    uint64_t client_max_body_size_ = 1024 * 1024;  // Default: 1MB
    std::map<uint16_t, std::string> error_pages_;
    std::vector<std::shared_ptr<Location>> locations_;
};

#endif