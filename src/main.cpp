#include "ConfigParser.hpp"
#include "ConfigValidator.hpp"
#include "ConfigPrinter.hpp"
#include "Config.hpp"
#include <iostream>
#include <fstream>

# define DEFAULT_CONFIG "./webserv.conf"

int main(int argc, char* argv[]) {
    (void) argc;
    try {
        const std::string path_to_config = argv[1] ? argv[1] : DEFAULT_CONFIG;
        std::ifstream config_file(path_to_config);
        if (!config_file) {
            std::cerr << "Error: Cannot open config file: " << argv[1] << "\n";
            return 1;
        }
        std::unique_ptr<Config> config = ConfigParser::parse(config_file);

        ConfigValidator::validate(*config);

        ConfigPrinter::print(std::cout, *config);

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