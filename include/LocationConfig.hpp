#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <vector>

class LocationConfig {
public:
    explicit LocationConfig(const std::string& path) : path_(path), autoindex_(false) {}

    // Builder-style setters
    LocationConfig& setRoot(const std::string& root) { 
        root_ = root; 
        return *this; 
    }
    
    LocationConfig& setIndex(const std::string& index) { 
        index_ = index; 
        return *this; 
    }
    
    LocationConfig& setRedirect(const std::string& redirect) { 
        redirect_ = redirect; 
        return *this; 
    }
    
    LocationConfig& setAllowedMethods(const std::vector<std::string>& methods) { 
        allowed_methods_ = methods; 
        return *this; 
    }
    
    LocationConfig& setAutoindex(bool enabled) { 
        autoindex_ = enabled; 
        return *this; 
    }
    
    // Getters
    const std::string& getPath() const { return path_; }
    const std::string& getRoot() const { return root_; }
    const std::string& getIndex() const { return index_; }
    const std::string& getRedirect() const { return redirect_; }
    const std::vector<std::string>& getAllowedMethods() const { return allowed_methods_; }
    bool getAutoindex() const { return autoindex_; }

private:
    std::string path_;
    std::string root_;
    std::string index_;
    std::string redirect_;
    std::vector<std::string> allowed_methods_;
    bool autoindex_;
};

#endif