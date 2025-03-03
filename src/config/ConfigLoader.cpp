#include "ConfigLoader.hpp"
#include "ConfigParser.hpp"
#include "ConfigValidator.hpp"
#include <fstream>
#include <iostream>

ConfigLoader::ConfigLoader() {}
ConfigLoader::~ConfigLoader() {}

std::unique_ptr<Config> ConfigLoader::load(const char* path) {
    const std::string path_to_config = path ? path : DEFAULT_CONFIG;
    std::ifstream config_file(path_to_config);
    if (!config_file) {
        throw std::runtime_error("Cannot open config file: " + path_to_config);
    }

    std::unique_ptr<Config> config = ConfigParser::parse(config_file);
    ConfigValidator::validate(*config);
    
    return config;
}