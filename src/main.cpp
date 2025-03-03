#include "ConfigLoader.hpp"
#include "ConfigPrinter.hpp"
#include "Config.hpp"
#include "ConfigParser.hpp"
#include "ConfigValidator.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    (void) argc;
    try {
        std::unique_ptr<Config> config = ConfigLoader::load(argv[1]);
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