#ifndef ICONFIGURATION_HPP
#define ICONFIGURATION_HPP

#include <string>
#include <vector>
#include <map>

// Forward declaration
class LocationConfig;

class IConfiguration {
public:
    virtual ~IConfiguration() = default;

    // Server configuration
    virtual uint16_t getPort() const = 0;
    virtual const std::string& getServerName() const = 0;
    virtual const std::string& getRoot() const = 0;
    virtual const std::string& getIndex() const = 0;
    virtual uint64_t getClientMaxBodySize() const = 0;
    
    // Error pages
    virtual const std::map<uint16_t, std::string>& getErrorPages() const = 0;
    
    // Locations
    virtual const std::vector<LocationConfig>& getLocations() const = 0;
};

#endif