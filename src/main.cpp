#include "ConfigParser.hpp"
#include "ConfigValidator.hpp"
#include <iostream>
#include <fstream>

// Helper function to print return directive information
void printReturnDirective(const Location::ReturnDirective& ret) {
    if (ret.type == Location::ReturnType::NONE) {
        return;
    }
    
    std::cout << "  Return: ";
    if (ret.type == Location::ReturnType::REDIRECT) {
        std::cout << ret.code << " -> " << ret.body << " (Redirect)\n";
    } else {
        std::cout << ret.code;
        if (!ret.body.empty()) {
            std::cout << " \"" << ret.body << "\"";
        }
        std::cout << " (Response)\n";
    }
}

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
            if (!location) continue;  // Skip null locations if any
            
            std::cout << "\nPath: " << location->getPath() << "\n";
            
            if (!location->getRoot().empty()) {
                std::cout << "  Root: " << location->getRoot() << "\n";
            }
            
            if (!location->getIndex().empty()) {
                std::cout << "  Index: " << location->getIndex() << "\n";
            }
            
            std::cout << "  Autoindex: " << (location->getAutoindex() ? "on" : "off") << "\n";
            
            if (!location->getAllowedMethods().empty()) {
                std::cout << "  Methods:";
                for (const auto& method : location->getAllowedMethods()) {
                    std::cout << " " << method;
                }
                std::cout << "\n";
            }

            if (location->hasReturn()) {
                printReturnDirective(location->getReturn());
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