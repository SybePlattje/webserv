#include "Config.hpp"
#include <iostream>
#include "Server.hpp"
#include <signal.h>

int main(int argc, char* argv[]) {
    ::signal(SIGPIPE, SIG_IGN);
    (void) argc;
    try {
        auto configs = ConfigLoader::load(argv[1]);
        ConfigPrinter::printConfigs(std::cout, configs);
        // Server server(config);
        // int nr = server.setupEpoll();
        // ConfigPrinter::print(std::cout, *config);
        // nr *= -1;
        // return nr;
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