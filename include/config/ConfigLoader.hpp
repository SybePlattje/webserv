#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include <memory>
#include <string>

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
     * @brief Loads and parses a configuration file
     *
     * Opens the specified file, reads its contents, and passes
     * it to the parser. Handles file operation errors and
     * provides appropriate error messages.
     *
     * @param path Path to the configuration file
     * @return Unique pointer to the parsed Config object
     * @throws std::runtime_error if file cannot be opened or read
     * @throws ConfigParser::ParseError if configuration syntax is invalid
     */
    static std::unique_ptr<Config> load(const char* path);

private:
    ConfigLoader() = delete;  // Static class - no instances allowed
};

#endif