#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include <memory>
#include <string>
#include <vector>

class Config;

/**
 * @brief Configuration loader for reading config files
 *
 * Handles loading and parsing of configuration files from disk.
 * Provides error handling for file operations and passes the
 * file content to the parser for processing.
 */
class ConfigLoader {
public:
    /**
     * @brief Loads and parses a configuration file containing multiple server blocks
     *
     * Opens the specified file, reads its contents, and passes
     * it to the parser. Handles file operation errors and
     * provides appropriate error messages.
     *
     * @param path Path to the configuration file
     * @return Vector of unique pointers to the parsed Config objects, one for each server block
     * @throws std::runtime_error if file cannot be opened or read
     * @throws ConfigParser::ParseError if configuration syntax is invalid
     */
    static std::vector<std::unique_ptr<Config>> load(const char* path);

private:
    ConfigLoader() = delete;  // Static class - no instances allowed
};

#endif