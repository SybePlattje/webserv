#include "Config.hpp"
#include <iostream>
#include "Server.hpp"
#include <signal.h>

int main(int argc, char* argv[]) {
    ::signal(SIGPIPE, SIG_IGN);
    (void) argc;
    try {
        std::vector<std::shared_ptr<Config>> configs = ConfigLoader::load(argv[1]);
        Server server(configs);
        int nr = server.setupEpoll();
        if (nr != 0)
            return nr * -1;
        nr = server.serverLoop();
        return nr * -1;

        //ConfigPrinter::printConfigs(std::cout, configs);
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