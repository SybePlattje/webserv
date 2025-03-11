#ifndef CONFIG_PRINTER_HPP
#define CONFIG_PRINTER_HPP

#include "Location.hpp"
#include <ostream>

class Config;

/**
 * @brief Configuration printer for human-readable output
 *
 * Provides formatted output of server configurations, including
 * server settings, error pages, and location blocks. Used for
 * configuration validation and debugging.
 */
class ConfigPrinter {
public:
    /**
     * @brief Prints complete configuration to output stream
     * @param out Output stream to write to
     * @param config Configuration to print
     */
    static void print(std::ostream& out, const Config& config);

private:
    ConfigPrinter() = delete;  // Static class - no instances allowed

    /**
     * @brief Prints server-wide settings
     * @param out Output stream to write to
     * @param config Configuration to print from
     */
    static void printServerInfo(std::ostream& out, const Config& config);

    /**
     * @brief Prints error page mappings
     * @param out Output stream to write to
     * @param config Configuration to print from
     */
    static void printErrorPages(std::ostream& out, const Config& config);

    /**
     * @brief Prints all location blocks
     * @param out Output stream to write to
     * @param config Configuration to print from
     */
    static void printLocations(std::ostream& out, const Config& config);

    /**
     * @brief Prints a single location block
     * @param out Output stream to write to
     * @param location Location block to print
     */
    static void printLocation(std::ostream& out, const Location& location);

    /**
     * @brief Prints return/redirect directive
     * @param out Output stream to write to
     * @param ret Return directive to print
     */
    static void printReturnDirective(std::ostream& out, const Location::ReturnDirective& ret);

    /**
     * @brief Prints list of allowed HTTP methods
     * @param out Output stream to write to
     * @param methods List of methods to print
     */
    static void printMethods(std::ostream& out, const std::vector<std::string>& methods);

    /**
     * @brief Prints CGI configuration
     * @param out Output stream to write to
     * @param cgi CGI configuration to print
     */
    static void printCGIConfig(std::ostream& out, const Location::CGIConfig& cgi);

    // Constants for formatting
    static constexpr const char* INDENT = "  ";     ///< Indentation for nested items
    static constexpr const char* NEWLINE = "\n";    ///< Line ending character
};

#endif