#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>

class Location {
public:
    Location(const std::string& path) : path_(path) {}
    ~Location() = default;

    // Getters
    const std::string& getPath() const { return path_; }
    const std::string& getRoot() const { return root_; }
    const std::string& getIndex() const { return index_; }
    const std::vector<std::string>& getAllowedMethods() const { return allowed_methods_; }
    bool getAutoindex() const { return autoindex_; }
    const std::string& getRedirect() const { return redirect_; }

private:
    friend class ConfigBuilder; // Only builder can modify location

    std::string path_;
    std::string root_;
    std::string index_ = "index.html";  // Default index
    std::vector<std::string> allowed_methods_ = {"GET"};  // Default: GET only
    bool autoindex_ = false;  // Default: autoindex off
    std::string redirect_;
};

#endif