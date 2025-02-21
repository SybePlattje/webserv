#ifndef CONFIG_VALIDATOR_HPP
#define CONFIG_VALIDATOR_HPP

#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include "IConfiguration.hpp"
#include "LocationConfig.hpp"

class ConfigValidator {
public:
    class ValidationException : public std::runtime_error {
    public:
        explicit ValidationException(const std::string& msg) : std::runtime_error(msg) {}
    };

    static void validateConfiguration(const IConfiguration& config) {
        validatePort(config.getPort());
        validatePath(config.getRoot());
        if (!config.getIndex().empty()) {
            validateFilename(config.getIndex());
        }
        validateErrorPages(config.getErrorPages());
        validateLocations(config.getLocations());
        validateServerName(config.getServerName());
        validateClientMaxBodySize(config.getClientMaxBodySize());
    }

private:
    ConfigValidator() = delete;
    
    static void validatePort(uint16_t port) {
        if (port == 0) {
            throw ValidationException("Invalid port number: 0");
        }
    }
    
    static void validatePath(const std::string& path) {
        if (path.empty()) {
            throw ValidationException("Path cannot be empty");
        }
        if (path[0] != '/') {
            throw ValidationException("Path must start with /: " + path);
        }
    }
    
    static void validateFilename(const std::string& filename) {
        if (filename.empty()) {
            throw ValidationException("Filename cannot be empty");
        }
        if (filename.find('/') == 0) {
            throw ValidationException("Index file should not start with /: " + filename);
        }
    }
    
    static void validateErrorPages(const std::map<uint16_t, std::string>& pages) {
        for (const auto& [code, path] : pages) {
            if (code < 400 || code > 599) {
                throw ValidationException("Invalid error code: " + std::to_string(code));
            }
            validatePath(path);  // Error pages must be absolute paths
        }
    }
    
    static void validateLocations(const std::vector<LocationConfig>& locations) {
        for (const auto& location : locations) {
            validatePath(location.getPath());  // Location paths must be absolute
            if (!location.getRoot().empty()) {
                validatePath(location.getRoot());  // Location root must be absolute
            }
            if (!location.getIndex().empty()) {
                validateFilename(location.getIndex());  // Location index is a filename
            }
            validateAllowedMethods(location.getAllowedMethods());
        }
    }
    
    static void validateServerName(const std::string& name) {
        if (name.empty()) {
            throw ValidationException("Server name cannot be empty");
        }
    }
    
    static void validateClientMaxBodySize(uint64_t size) {
        if (size == 0) {
            throw ValidationException("Client max body size cannot be 0");
        }
    }
    
    static void validateAllowedMethods(const std::vector<std::string>& methods) {
        static const std::vector<std::string> valid_methods = {"GET", "POST", "DELETE", "PUT", "HEAD"};
        for (const auto& method : methods) {
            if (std::find(valid_methods.begin(), valid_methods.end(), method) == valid_methods.end()) {
                throw ValidationException("Invalid HTTP method: " + method);
            }
        }
    }
};

#endif