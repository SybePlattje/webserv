#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "config/ConfigLoader.hpp"
#include "config/ConfigPrinter.hpp"
#include "config/ConfigParser.hpp"
#include "config/ConfigValidator.hpp"
#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <memory>

// Forward declaration
class Location;

/**
 * @brief Configuration storage for the web server
 * 
 * This class stores the complete configuration for a web server instance,
 * including server-wide settings and location-specific configurations.
 * It follows an immutable pattern where all modifications must go through
 * the ConfigBuilder class.
 */
class Config {
public:
    Config() = default;
    ~Config() = default;

    /**
     * @return Port number the server listens on
     */
    uint16_t getPort() const { return port_; }

    /**
     * @return Server name used in HTTP headers
     */
    const std::string& getServerName() const { return server_name_; }

    /**
     * @return Root directory for serving files
     */
    const std::string& getRoot() const { return root_; }

    /**
     * @return Default index file when requesting a directory
     */
    const std::string& getIndex() const { return index_; }

    /**
     * @return Maximum allowed size for client request bodies in bytes
     */
    uint64_t getClientMaxBodySize() const { return client_max_body_size_; }

    /**
     * @return Map of HTTP error codes to their custom error page paths
     */
    const std::map<uint16_t, std::string>& getErrorPages() const { return error_pages_; }

    /**
     * @return List of all configured location blocks
     */
    const std::vector<std::shared_ptr<Location>>& getLocations() const { return locations_; }

private:
    // Only ConfigBuilder can modify the configuration to ensure consistency
    friend class ConfigBuilder;

    // Server settings with sensible defaults
    uint16_t port_ = 9999;                      // Default port for development
    std::string server_name_ = "localhost";      // Default server name
    std::string root_ = "/";                     // Root directory
    std::string index_ = "index.html";           // Standard index filename
    uint64_t client_max_body_size_ = 1024*1024; // 1MB default body size limit

    // Custom error pages mapping (code -> page path)
    std::map<uint16_t, std::string> error_pages_;

    // List of location blocks defining URL-specific behaviors
    std::vector<std::shared_ptr<Location>> locations_;
};

#endif