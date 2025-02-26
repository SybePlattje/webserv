#include "ConfigParser.hpp"
#include "ConfigValidator.hpp"
#include "ConfigPrinter.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    try {
        std::ifstream config_file(argv[1]);
        if (!config_file) {
            std::cerr << "Error: Cannot open config file: " << argv[1] << "\n";
            return 1;
        }
        std::unique_ptr<Configuration> config = ConfigParser::parse(config_file);

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