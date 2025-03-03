#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <optional>

class Location {
public:
    // Types of return directives
    enum class ReturnType {
        NONE,           // No return directive
        REDIRECT,       // 301, 302, 303, 307, 308 redirects
        RESPONSE        // Direct response (200, 400, 403, 404, 405)
    };

    struct ReturnDirective {
        ReturnType type;
        unsigned int code;
        std::string body;     // URL for redirects, message for responses

        ReturnDirective() 
            : type(ReturnType::NONE), code(0) {}
        
        ReturnDirective(ReturnType t, unsigned int c, const std::string& b) 
            : type(t), code(c), body(b) {}

        bool isRedirect() const {
            return type == ReturnType::REDIRECT;
        }

        bool isResponse() const {
            return type == ReturnType::RESPONSE;
        }
    };

    Location(const std::string& path);
    ~Location() = default;

    // Copy constructor
    Location(const Location& other);

    // Copy assignment operator
    Location& operator=(const Location& other);

    // Getters
    const std::string& getPath() const;
    const std::string& getRoot() const;
    const std::string& getIndex() const;
    const std::vector<std::string>& getAllowedMethods() const;
    bool getAutoindex() const;
    const ReturnDirective& getReturn() const;

    // Return directive helper methods
    bool hasReturn() const;
    bool hasRedirect() const;
    bool hasResponse() const;

private:
    friend class ConfigBuilder; // Only builder can modify location

    std::string path_;
    std::string root_;
    std::string index_ = "index.html";  // Default index
    std::vector<std::string> allowed_methods_ = {"GET"};  // Default: GET only
    bool autoindex_ = false;  // Default: autoindex off
    ReturnDirective return_directive_;

    // Helper method to check if a status code is valid for redirects
    static bool isValidRedirectCode(unsigned int code) {
        return code == 301 || code == 302 || code == 303 || 
               code == 307 || code == 308;
    }

    // Helper method to check if a status code is valid for responses
    static bool isValidResponseCode(unsigned int code) {
        return code == 200 || code == 400 || code == 403 || 
               code == 404 || code == 405;
    }
};

#endif