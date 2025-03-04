#ifndef CONFIG_VALIDATOR_HPP
#define CONFIG_VALIDATOR_HPP

#include "Location.hpp"
#include <regex>
#include <stdexcept>
#include <set>

class Config;

class ConfigValidator {
public:
    class ValidationError : public std::runtime_error {
    public:
        explicit ValidationError(const std::string& msg) : std::runtime_error(msg) {}
    };

    // Constants for validation
    static constexpr size_t MAX_BODY_SIZE = 1024 * 1024 * 1024; // 1GB
    static constexpr size_t MAX_PATH_LENGTH = 4096;

    // Main validation method
    static void validate(const Config& config);

private:
    ConfigValidator() = delete;  // Static class

    // Path validation
    static void validatePath(const std::string& path, const std::string& context);
    static void validateFilename(const std::string& filename, const std::string& context);
    static bool isValidPath(const std::string& path);
    static bool isValidFilename(const std::string& filename);

    // HTTP validation
    static void validatePort(uint16_t port);
    static void validateMethod(const std::string& method);
    static void validateMethods(const std::vector<std::string>& methods);
    static void validateErrorCode(uint16_t code);
    static void validateServerName(const std::string& name);
    static void validateClientMaxBodySize(uint64_t size);

    // Return directive validation
    static void validateReturnDirective(const Location::ReturnDirective& ret, const std::string& context);
    static bool isValidRedirectCode(unsigned int code);
    static bool isValidResponseCode(unsigned int code);

    // Location validation
    static void validateLocations(const std::vector<std::shared_ptr<Location>>& locations);
    static void validateLocation(const Location& location);
    static void validateLocationPaths(const std::vector<std::shared_ptr<Location>>& locations);

    // Regex patterns
    static const std::regex path_pattern_;
    static const std::regex filename_pattern_;
    static const std::regex server_name_pattern_;
    static const std::set<std::string> valid_methods_;
};

// Define static regex patterns
inline const std::regex ConfigValidator::path_pattern_("^[/a-zA-Z0-9._-]+$");
inline const std::regex ConfigValidator::filename_pattern_("^[a-zA-Z0-9._-]+$");
inline const std::regex ConfigValidator::server_name_pattern_("^[a-zA-Z0-9.-]+$");

// Define valid HTTP methods
inline const std::set<std::string> ConfigValidator::valid_methods_ = {
    "GET", "POST", "DELETE", "PUT", "HEAD", "OPTIONS", "TRACE"
};

#endif