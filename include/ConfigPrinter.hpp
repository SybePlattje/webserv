#ifndef CONFIG_PRINTER_HPP
#define CONFIG_PRINTER_HPP

#include "Configuration.hpp"
#include "Location.hpp"
#include <ostream>

class ConfigPrinter {
public:
    // Print the entire configuration
    static void print(std::ostream& out, const Configuration& config);

private:
    ConfigPrinter() = delete;  // Static class

    // Helper methods for printing different parts
    static void printServerInfo(std::ostream& out, const Configuration& config);
    static void printErrorPages(std::ostream& out, const Configuration& config);
    static void printLocations(std::ostream& out, const Configuration& config);
    static void printLocation(std::ostream& out, const Location& location);
    static void printReturnDirective(std::ostream& out, const Location::ReturnDirective& ret);
    static void printMethods(std::ostream& out, const std::vector<std::string>& methods);

    // Constants for formatting
    static constexpr const char* INDENT = "  ";
    static constexpr const char* NEWLINE = "\n";
};

#endif