#include "Config.hpp"
#include <fstream>
#include <iostream>

// Default configuration file path if none specified
static const char* DEFAULT_CONFIG = "webserv.conf";

std::vector<std::unique_ptr<Config>> ConfigLoader::load(const char* path) {
    // Use provided path or fall back to default
    const std::string path_to_config = path ? path : DEFAULT_CONFIG;

    // Attempt to open configuration file
    std::ifstream config_file(path_to_config);
    if (!config_file) {
        throw std::runtime_error("Cannot open config file: " + path_to_config);
    }

    try {
        // Parse configuration from file
        std::vector<std::unique_ptr<Config>> configs = ConfigParser::parse(config_file);

        // Validate each server configuration
        for (const auto& config : configs) {
            ConfigValidator::validate(*config);
        }
        
        return configs;
    } catch (const ConfigParser::ParseError& e) {
        // Add file path context to parsing errors
        throw std::runtime_error("Error in " + path_to_config + ": " + e.what());
    } catch (const std::exception& e) {
        // Add file path context to validation errors
        throw std::runtime_error("Error in " + path_to_config + ": " + e.what());
    }
}