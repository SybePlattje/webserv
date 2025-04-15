#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "cgi/CGIExecutor.hpp"
#include "config/Location.hpp"
#include <string>
#include <map>
#include <memory>

/**
 * @brief High-level CGI request handling and response formatting
 * 
 * This class orchestrates CGI processing by:
 * - Using Location config to validate CGI requests
 * - Setting up CGI environment variables
 * - Managing script execution via CGIExecutor
 * - Formatting responses according to HTTP spec
 */
class CGIHandler {
public:
    /**
     * @brief Initialize handler with location configuration
     * @param location Location block containing CGI configuration
     */
    explicit CGIHandler(const Location& location);
    ~CGIHandler() = default;

    /**
     * @brief Process a CGI request
     * @param script_path Path to the CGI script
     * @param request_method HTTP method (GET/POST)
     * @param request_body Request body data (for POST)
     * @param query_string Query string from URL (for GET)
     * @param server_name Server's hostname
     * @param server_port Server's port
     * @return Pair of {status_code, response_content}
     * @throw std::runtime_error on processing failure
     */
    std::pair<int, std::string> handleRequest(
        const std::string& script_path,
        const std::string& request_method,
        const std::string& request_body,
        const std::string& query_string,
        const std::string& server_name,
        uint16_t server_port);

private:
    CGIExecutor executor_;
    const Location& location_;

    /**
     * @brief Get interpreter for script based on extension
     * @param script_path Path to script
     * @return Path to appropriate interpreter
     * @throw std::runtime_error if no matching interpreter
     */
    std::string getInterpreter(const std::string& script_path) const;

    /**
     * @brief Set up CGI environment variables
     * @param script_path Path to CGI script
     * @param request_method HTTP method
     * @param query_string Query string from URL
     * @param server_name Server's hostname
     * @param server_port Server's port
     * @param content_length Length of request body
     * @return Map of environment variables
     */
    std::map<std::string, std::string> setupEnvironment(
        const std::string& script_path,
        const std::string& request_method,
        const std::string& query_string,
        const std::string& server_name,
        uint16_t server_port,
        size_t content_length) const;

    /**
     * @brief Get script extension (including dot)
     * @param path Path to script
     * @return Extension (e.g., ".py")
     */
    std::string getExtension(const std::string& path) const;
};

#endif // CGI_HANDLER_HPP