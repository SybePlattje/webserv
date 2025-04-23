#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <optional>
#include <regex>

/**
 * @brief Location block configuration for URL-specific behavior
 *
 * Represents a location block in the server configuration, defining
 * how requests to specific URLs should be handled. This includes
 * settings for file serving, request methods, CGI processing, and
 * redirects/responses.
 */
class Location {
public:
    /**
     * @brief Types of location matching
     * 
     * Locations are matched in this order:
     * 1. Exact match (=)
     * 2. Preferential prefix (^~)
     * 3. Regular expressions (~, ~*)
     * 4. Prefix match (no modifier) [default]
     */
    enum class MatchType {
        EXACT,              ///< Exact path match (=)
        PREFIX,            ///< Prefix match (no modifier, default)
        PREFERENTIAL_PREFIX,///< Preferential prefix match (^~)
        REGEX,              ///< Case-sensitive regex match (~)
        REGEX_INSENSITIVE   ///< Case-insensitive regex match (~*)
    };

    /**
     * @brief Types of return directives for response handling
     */
    enum class ReturnType {
        NONE,     ///< No return directive specified
        REDIRECT, ///< HTTP redirect (301, 302, 303, 307, 308)
        RESPONSE  ///< Direct response (200, 400, 403, 404, 405)
    };

    /**
     * @brief Configuration for return/redirect directives
     */
    struct ReturnDirective {
        ReturnType type;      ///< Type of return (none/redirect/response)
        unsigned int code;    ///< HTTP status code
        std::string body;     ///< URL for redirects, message for responses

        ReturnDirective() 
            : type(ReturnType::NONE), code(0) {}
        
        ReturnDirective(ReturnType t, unsigned int c, const std::string& b) 
            : type(t), code(c), body(b) {}

        /**
         * @return true if this is a redirect directive
         */
        bool isRedirect() const {
            return type == ReturnType::REDIRECT;
        }

        /**
         * @return true if this is a response directive
         */
        bool isResponse() const {
            return type == ReturnType::RESPONSE;
        }
    };

    /**
     * @brief Configuration for CGI processing
     */
    struct CGIConfig {
        std::vector<std::string> interpreters; ///< Paths to CGI interpreters
        std::vector<std::string> extensions;   ///< File extensions to handle as CGI

        /**
         * @return true if CGI is enabled (has both interpreters and extensions)
         */
        bool isEnabled() const {
            return !interpreters.empty() && !extensions.empty();
        }
    };

    /**
     * @brief Creates a location block for the specified URL path
     * @param path URL path this location handles
     * @param type Type of location matching
     */
    Location(const std::string& path, MatchType type = MatchType::PREFIX);
    ~Location() = default;

    // Copy operations
    Location(const Location& other);
    Location& operator=(const Location& other);

    // Getters
    /**
     * @return URL path this location handles
     */
    const std::string& getPath() const;

    /**
     * @return Type of location matching
     */
    MatchType getMatchType() const;

    /**
     * @return Root directory for this location
     */
    const std::string& getRoot() const;

    /**
     * @return Index file for directory requests
     */
    const std::string& getIndex() const;

    /**
     * @return List of allowed HTTP methods
     */
    const std::vector<std::string>& getAllowedMethods() const;

    /**
     * @return true if directory listing is enabled
     */
    bool getAutoindex() const;

    /**
     * @return Return/redirect configuration
     */
    const ReturnDirective& getReturn() const;

    /**
     * @return CGI processing configuration
     */
    const CGIConfig& getCGIConfig() const;

    /**
     * @return Compiled regex pattern if this is a regex location
     */
    const std::regex& getRegex() const;

    // Status checks
    /**
     * @return true if a return directive is set
     */
    bool hasReturn() const;

    /**
     * @return true if this location performs a redirect
     */
    bool hasRedirect() const;

    /**
     * @return true if this location sends a direct response
     */
    bool hasResponse() const;

    /**
     * @return true if CGI processing is enabled
     */
    bool hasCGI() const;

    /**
     * @brief Checks if a file extension should be handled as CGI
     * @param ext File extension to check (including dot)
     * @return true if the extension should be handled as CGI
     */
    bool isCGIExtension(const std::string& ext) const;

private:
    friend class ConfigBuilder; // Only builder can modify location

    // Member variables
    std::string path_;                              ///< URL path this location handles
    MatchType match_type_;                          ///< Type of location matching
    std::string root_;                              ///< Root directory for serving files
    std::string index_;                             ///< Default index file
    std::vector<std::string> allowed_methods_{"GET"}; ///< Default: GET only
    bool autoindex_ = false;                        ///< Default: directory listing off
    ReturnDirective return_directive_;              ///< Return/redirect configuration
    CGIConfig cgi_config_;                          ///< CGI processing settings
    std::regex regex_;                              ///< Compiled regex pattern for regex locations

    /**
     * @brief Validates a redirect status code
     * @param code HTTP status code
     * @return true if code is valid for redirects
     */
    static bool isValidRedirectCode(unsigned int code) {
        return code == 301 || code == 302 || code == 303 || 
               code == 307 || code == 308;
    }

    /**
     * @brief Validates a response status code
     * @param code HTTP status code
     * @return true if code is valid for responses
     */
    static bool isValidResponseCode(unsigned int code) {
        return code == 200 || code == 400 || code == 403 || 
               code == 404 || code == 405;
    }
};

#endif // LOCATION_HPP