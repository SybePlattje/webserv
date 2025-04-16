#include "Config.hpp"

void ConfigValidator::validateConfigs(const std::vector<std::unique_ptr<Config>>& configs) {
    if (configs.empty()) {
        throw ValidationError("No server configurations found");
    }

    // First validate each individual server config
    for (const auto& config : configs) {
        validate(*config);
    }

    // Then validate inter-server requirements
    validateNoDuplicatePorts(configs);
}

void ConfigValidator::validateNoDuplicatePorts(const std::vector<std::unique_ptr<Config>>& configs) {
    std::set<uint16_t> used_ports;
    for (const auto& config : configs) {
        uint16_t port = config->getPort();
        if (!used_ports.insert(port).second) {
            throw ValidationError("Duplicate port number found: " + std::to_string(port));
        }
    }
}

void ConfigValidator::validate(const Config& config) {
    // Validate server settings
    validatePort(config.getPort());
    validateServerName(config.getServerName());
    validatePath(config.getRoot(), "server root");
    validateFilename(config.getIndex(), "server index");
    validateClientMaxBodySize(config.getClientMaxBodySize());

    // Validate error pages
    for (const auto& [code, path] : config.getErrorPages()) {
        validateErrorCode(code);
        validatePath(path, "error page");
    }

    // Validate locations
    validateLocations(config.getLocations());
}

void ConfigValidator::validatePath(const std::string& path, const std::string& context) {
    if (path.empty()) {
        throw ValidationError(context + ": Path cannot be empty");
    }
    if (path[0] != '/') {
        throw ValidationError(context + ": Path must start with /: " + path);
    }
    if (path.length() > MAX_PATH_LENGTH) {
        throw ValidationError(context + ": Path exceeds maximum length: " + path);
    }
    if (!std::regex_match(path, path_pattern_)) {
        throw ValidationError(context + ": Invalid characters in path: " + path);
    }
}

void ConfigValidator::validateFilename(const std::string& filename, const std::string& context) {
    if (filename.empty()) {
        throw ValidationError(context + ": Filename cannot be empty");
    }
    if (filename[0] == '/') {
        throw ValidationError(context + ": Filename should not start with /: " + filename);
    }
    if (!std::regex_match(filename, filename_pattern_)) {
        throw ValidationError(context + ": Invalid characters in filename: " + filename);
    }
}

void ConfigValidator::validatePort(uint16_t port) {
    if (port == 0) {
        throw ValidationError("Port number cannot be 0");
    }
}

void ConfigValidator::validateMethod(const std::string& method) {
    if (valid_methods_.find(method) == valid_methods_.end()) {
        throw ValidationError("Invalid HTTP method: " + method);
    }
}

void ConfigValidator::validateMethods(const std::vector<std::string>& methods) {
    if (methods.empty()) {
        throw ValidationError("At least one HTTP method must be specified");
    }
    for (const auto& method : methods) {
        validateMethod(method);
    }
}

void ConfigValidator::validateErrorCode(uint16_t code) {
    if (code < 400 || code > 599) {
        throw ValidationError("Invalid error code: " + std::to_string(code));
    }
}

void ConfigValidator::validateServerName(const std::string& name) {
    if (name.empty()) {
        throw ValidationError("Server name cannot be empty");
    }
    if (!std::regex_match(name, server_name_pattern_)) {
        throw ValidationError("Invalid server name: " + name);
    }
}

void ConfigValidator::validateClientMaxBodySize(uint64_t size) {
    if (size == 0) {
        throw ValidationError("Client max body size cannot be 0");
    }
    if (size > MAX_BODY_SIZE) {
        throw ValidationError("Client max body size exceeds maximum allowed (" + 
            std::to_string(MAX_BODY_SIZE) + " bytes)");
    }
}

void ConfigValidator::validateReturnDirective(const Location::ReturnDirective& ret, const std::string& context) {
    if (ret.type == Location::ReturnType::NONE) {
        return;  // No return directive to validate
    }

    // Validate redirect
    if (ret.type == Location::ReturnType::REDIRECT) {
        if (ret.body.empty()) {
            throw ValidationError(context + ": Redirect requires a URL");
        }
        if (!isValidRedirectCode(ret.code)) {
            throw ValidationError(context + ": Invalid redirect code: " + std::to_string(ret.code));
        }
        validatePath(ret.body, context + " redirect URL");
    }
    // Validate response
    else if (ret.type == Location::ReturnType::RESPONSE) {
        if (!isValidResponseCode(ret.code)) {
            throw ValidationError(context + ": Invalid response code: " + std::to_string(ret.code));
        }
        // Message is optional for responses, no validation needed
    }
}

bool ConfigValidator::isValidRedirectCode(unsigned int code) {
    return code == 301 || code == 302 || code == 303 || code == 307 || code == 308;
}

bool ConfigValidator::isValidResponseCode(unsigned int code) {
    return code == 200 || code == 400 || code == 403 || code == 404 || code == 405;
}

void ConfigValidator::validateLocations(const std::vector<std::shared_ptr<Location>>& locations) {
    if (locations.empty()) {
        throw ValidationError("At least one location block is required");
    }

    validateLocationPaths(locations);

    for (const auto& location : locations) {
        validateLocation(*location);
    }
}

void ConfigValidator::validateLocation(const Location& location) {
    // Skip path validation for regex locations
    if (location.getMatchType() != Location::MatchType::REGEX &&
        location.getMatchType() != Location::MatchType::REGEX_INSENSITIVE) {
        validatePath(location.getPath(), "location path");
    }
    
    if (!location.getRoot().empty()) {
        validatePath(location.getRoot(), "location root");
    }
    
    if (!location.getIndex().empty()) {
        validateFilename(location.getIndex(), "location index");
    }
    
    validateMethods(location.getAllowedMethods());

    // Validate return directive if present
    if (location.hasReturn()) {
        validateReturnDirective(location.getReturn(), 
                             "Location " + location.getPath());
    }
}

void ConfigValidator::validateLocationPaths(const std::vector<std::shared_ptr<Location>>& locations) {
    std::set<std::string> paths;
    for (const auto& location : locations) {
        const std::string& path = location->getPath();
        if (!paths.insert(path).second) {
            throw ValidationError("Duplicate location path: " + path);
        }
    }
}