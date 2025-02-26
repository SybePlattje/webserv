#include "ConfigParser.hpp"
#include "ConfigValidator.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    try {
        // Open config file
        std::ifstream config_file(argv[1]);
        if (!config_file) {
            std::cerr << "Error: Cannot open config file: " << argv[1] << "\n";
            return 1;
        }

        // Parse configuration
        std::unique_ptr<Configuration> config = ConfigParser::parse(config_file);

        // Validate configuration
        ConfigValidator::validate(*config);

        // Configuration is valid, print summary
        std::cout << "Configuration loaded successfully:\n"
                  << "Port: " << config->getPort() << "\n"
                  << "Server name: " << config->getServerName() << "\n"
                  << "Root: " << config->getRoot() << "\n"
                  << "Index: " << config->getIndex() << "\n"
                  << "Client max body size: " << config->getClientMaxBodySize() << " bytes\n"
                  << "Number of locations: " << config->getLocations().size() << "\n";

        // Print error pages
        std::cout << "\nError pages:\n";
        for (const auto& [code, path] : config->getErrorPages()) {
            std::cout << code << " -> " << path << "\n";
        }

        // Print locations
        std::cout << "\nLocations:\n";
        for (const auto& location : config->getLocations()) {
            std::cout << "\nPath: " << location->getPath() << "\n"
                      << "  Root: " << location->getRoot() << "\n"
                      << "  Index: " << location->getIndex() << "\n"
                      << "  Autoindex: " << (location->getAutoindex() ? "on" : "off") << "\n"
                      << "  Methods:";
            
            for (const auto& method : location->getAllowedMethods()) {
                std::cout << " " << method;
            }
            std::cout << "\n";

            if (!location->getRedirect().empty()) {
                std::cout << "  Redirect: " << location->getRedirect() << "\n";
            }
        }

        return 0;
    }
    catch (const ConfigParser::ParseError& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return 1;
    }
    catch (const ConfigValidator::ValidationError& e) {
        std::cerr << "Validation error: " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}