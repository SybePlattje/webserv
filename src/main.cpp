#include "ConfigParser.hpp"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    try {
        const char* config_path = argc > 1 ? argv[1] : "./webserv.conf";
        
        // Parse configuration
        auto config = ConfigParser::parseFile(config_path);
        
        // Print configuration
        std::cout << "Server Configuration:\n"
                  << "-------------------\n"
                  << "Port: " << config->getPort() << "\n"
                  << "Server Name: " << config->getServerName() << "\n"
                  << "Root: " << config->getRoot() << "\n"
                  << "Index: " << config->getIndex() << "\n"
                  << "Client Max Body Size: " << config->getClientMaxBodySize() << " bytes\n\n";

        // Print locations
        std::cout << "Locations:\n"
                  << "---------\n";
        for (const auto& location : config->getLocations()) {
            std::cout << "Path: " << location.getPath() << "\n"
                     << "  Root: " << location.getRoot() << "\n"
                     << "  Index: " << location.getIndex() << "\n"
                     << "  Autoindex: " << (location.getAutoindex() ? "on" : "off") << "\n"
                     << "  Allowed Methods: ";
            
            for (const auto& method : location.getAllowedMethods()) {
                std::cout << method << " ";
            }
            std::cout << "\n\n";
        }

        // Print error pages
        std::cout << "Error Pages:\n"
                  << "-----------\n";
        for (const auto& [code, path] : config->getErrorPages()) {
            std::cout << code << ": " << path << "\n";
        }

        return 0;
        
    } catch (const ConfigParser::ParserException& e) {
        std::cerr << "Configuration parsing error: " << e.what() << std::endl;
        return 1;
    } catch (const ConfigValidator::ValidationException& e) {
        std::cerr << "Configuration validation error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}
