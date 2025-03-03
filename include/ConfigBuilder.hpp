#ifndef CONFIG_BUILDER_HPP
#define CONFIG_BUILDER_HPP

#include "Config.hpp"
#include "Location.hpp"
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

class ConfigBuilder {
public:
    ConfigBuilder();

    // Server configuration methods
    ConfigBuilder& setPort(uint16_t port);
    ConfigBuilder& setServerName(const std::string& name);
    ConfigBuilder& setRoot(const std::string& root);
    ConfigBuilder& setIndex(const std::string& index);
    ConfigBuilder& setClientMaxBodySize(uint64_t size);
    ConfigBuilder& addErrorPage(uint16_t code, const std::string& page);

    // Location configuration methods
    void startLocation(const std::string& path);
    void setLocationRoot(const std::string& root);
    void setLocationIndex(const std::string& index);
    void setLocationMethods(const std::vector<std::string>& methods);
    void setLocationAutoindex(bool enabled);
    void setLocationRedirect(unsigned int code, const std::string& url);
    void setLocationResponse(unsigned int code, const std::string& message = "");
    void endLocation();

    // Build method
    std::unique_ptr<Config> build();

private:
    std::unique_ptr<Config> config_;
    std::shared_ptr<Location> current_location_;
};

#endif