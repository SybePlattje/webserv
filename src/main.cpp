#include "Config.hpp"
#include <iostream>
#include "Server.hpp"
#include <signal.h>

int main(int argc, char* argv[]) {
    ::signal(SIGPIPE, SIG_IGN);
	(void) argc;
    try {
        std::unique_ptr<Config> config = ConfigLoader::load(argv[1]);
        Server server(*config);
        server.setupEpoll();
        // ConfigPrinter::print(std::cout, *config);
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