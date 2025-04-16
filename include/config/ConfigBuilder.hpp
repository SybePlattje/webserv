#ifndef CONFIG_BUILDER_HPP
#define CONFIG_BUILDER_HPP

#include "Location.hpp"
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

class Config;

/**
 * @brief Builder for creating server configurations
 *
 * Implements the Builder pattern to construct Config objects,
 * providing a fluent interface for server settings and validation
 * of configuration values during construction.
 */
class ConfigBuilder {
public:
    ConfigBuilder();

    // Server configuration methods
    /**
     * @brief Sets the port number for the server
     * @param port Port number (1-65535)
     * @return Reference to this builder for method chaining
     * @throws std::runtime_error if port is invalid
     */
    ConfigBuilder& setPort(uint16_t port);

    /**
     * @brief Sets the server name for HTTP headers
     * @param name Server name
     * @return Reference to this builder for method chaining
     */
    ConfigBuilder& setServerName(const std::string& name);

    /**
     * @brief Sets the root directory for serving files
     * @param root Root directory path
     * @return Reference to this builder for method chaining
     */
    ConfigBuilder& setRoot(const std::string& root);

    /**
     * @brief Sets the default index file
     * @param index Index filename
     * @return Reference to this builder for method chaining
     */
    ConfigBuilder& setIndex(const std::string& index);

    /**
     * @brief Sets maximum allowed client request body size
     * @param size Maximum size in bytes
     * @return Reference to this builder for method chaining
     */
    ConfigBuilder& setClientMaxBodySize(uint64_t size);

    /**
     * @brief Adds a custom error page mapping
     * @param code HTTP error code (400-599)
     * @param page Path to error page
     * @return Reference to this builder for method chaining
     * @throws std::runtime_error if code is invalid
     */
    ConfigBuilder& addErrorPage(uint16_t code, const std::string& page);

    // Location configuration methods
    /**
     * @brief Starts a new location block configuration
     * @param path URL path for this location
     * @param type Type of location matching (defaults to PREFIX)
     * @throws std::runtime_error if another location is currently being configured
     * 
     * Location types are matched in this order:
     * 1. Exact match (=)
     * 2. Preferential prefix (^~)
     * 3. Regular expressions (~, ~*)
     * 4. Prefix match (no modifier) [default]
     */
    void startLocation(const std::string& path, Location::MatchType type = Location::MatchType::PREFIX);

    /**
     * @brief Sets root directory for current location
     * @param root Directory path
     * @throws std::runtime_error if no location is being configured
     */
    void setLocationRoot(const std::string& root);

    /**
     * @brief Sets index file for current location
     * @param index Index filename
     * @throws std::runtime_error if no location is being configured
     */
    void setLocationIndex(const std::string& index);

    /**
     * @brief Sets allowed HTTP methods for current location
     * @param methods List of HTTP methods
     * @throws std::runtime_error if no location is being configured
     */
    void setLocationMethods(const std::vector<std::string>& methods);

    /**
     * @brief Enables/disables directory listing for current location
     * @param enabled Whether autoindex is enabled
     * @throws std::runtime_error if no location is being configured
     */
    void setLocationAutoindex(bool enabled);

    /**
     * @brief Configures a redirect for current location
     * @param code HTTP redirect code (301, 302, 303, 307, 308)
     * @param url Redirect target URL
     * @throws std::runtime_error if no location is being configured or code is invalid
     */
    void setLocationRedirect(unsigned int code, const std::string& url);

    /**
     * @brief Configures a response for current location
     * @param code HTTP status code (200, 400, 403, 404, 405)
     * @param message Optional response message
     * @throws std::runtime_error if no location is being configured or code is invalid
     */
    void setLocationResponse(unsigned int code, const std::string& message = "");

    /**
     * @brief Sets CGI interpreter paths for current location
     * @param interpreters List of paths to CGI interpreters
     * @throws std::runtime_error if no location is being configured
     */
    void setLocationCGIPath(const std::vector<std::string>& interpreters);

    /**
     * @brief Sets CGI file extensions for current location
     * @param extensions List of file extensions to handle as CGI
     * @throws std::runtime_error if no location is being configured
     */
    void setLocationCGIExt(const std::vector<std::string>& extensions);

    /**
     * @brief Finalizes current location configuration
     * @throws std::runtime_error if no location is being configured
     */
    void endLocation();

    /**
     * @brief Builds and returns the completed configuration
     * @return Unique pointer to the built Config object
     */
    std::unique_ptr<Config> build();

private:
    /**
     * @brief Ensures a location block is being configured
     * @param method Name of the calling method for error messages
     * @throws std::runtime_error if no location is being configured
     */
    void ensureLocationContext(const std::string& method) const {
        if (!current_location_) {
            throw std::runtime_error(method + " called outside location context");
        }
    }

    std::unique_ptr<Config> config_;             ///< Configuration being built
    std::shared_ptr<Location> current_location_; ///< Location currently being configured
};

#endif