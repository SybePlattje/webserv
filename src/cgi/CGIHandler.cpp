#include "cgi/CGIHandler.hpp"
#include <filesystem>
#include <sstream>
#include <stdexcept>

CGIHandler::CGIHandler(const Location& location)
    : location_(location)
{
    if (!location.hasCGI()) {
        throw std::runtime_error("Location does not have CGI configuration");
    }
}

std::pair<int, std::string> CGIHandler::handleRequest(
    const std::string& script_path,
    const std::string& request_method,
    const std::string& request_body,
    const std::string& query_string,
    const std::string& server_name,
    uint16_t server_port)
{
    // Get interpreter for this script type
    std::string interpreter = getInterpreter(script_path);

    // Set up environment variables
    auto env_vars = setupEnvironment(
        script_path,
        request_method,
        query_string,
        server_name,
        server_port,
        request_body.length()
    );

    // Execute the script
    return executor_.execute(interpreter, script_path, request_body, env_vars);
}

std::string CGIHandler::getInterpreter(const std::string& script_path) const
{
    std::string ext = getExtension(script_path);
    const auto& config = location_.getCGIConfig();
    
    // Match extension to interpreter
    for (size_t i = 0; i < config.extensions.size(); ++i) {
        if (config.extensions[i] == ext) {
            if (i < config.interpreters.size()) {
                return config.interpreters[i];
            }
        }
    }
    
    throw std::runtime_error("No interpreter found for extension: " + ext);
}

std::map<std::string, std::string> CGIHandler::setupEnvironment(
    const std::string& script_path,
    const std::string& request_method,
    const std::string& query_string,
    const std::string& server_name,
    uint16_t server_port,
    size_t content_length) const
{
    std::map<std::string, std::string> env;
    
    // Required CGI variables per CGI/1.1 spec
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["SERVER_SOFTWARE"] = "webserv/1.0";
    env["SERVER_NAME"] = server_name;
    env["SERVER_PORT"] = std::to_string(server_port);
    env["REQUEST_METHOD"] = request_method;
    env["SCRIPT_NAME"] = script_path;
    env["PATH_INFO"] = "";  // We don't support path info
    env["PATH_TRANSLATED"] = script_path;

    // Optional but commonly used variables
    if (!query_string.empty()) {
        env["QUERY_STRING"] = query_string;
    }

    if (content_length > 0) {
        env["CONTENT_LENGTH"] = std::to_string(content_length);
        env["CONTENT_TYPE"] = "multipart/form-data";
    }

    return env;
}

std::string CGIHandler::getExtension(const std::string& path) const
{
    std::filesystem::path fs_path(path);
    std::string ext = fs_path.extension();
    return ext;
}