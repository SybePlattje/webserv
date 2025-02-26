#ifndef CONFIG_BUILDER_HPP
#define CONFIG_BUILDER_HPP

#include "Configuration.hpp"
#include "Location.hpp"
#include <memory>

class ConfigBuilder {
public:
    ConfigBuilder() : config_(new Configuration()) {}

    // Server configuration methods
    ConfigBuilder& setPort(uint16_t port) {
        config_->port_ = port;
        return *this;
    }

    ConfigBuilder& setServerName(const std::string& name) {
        config_->server_name_ = name;
        return *this;
    }

    ConfigBuilder& setRoot(const std::string& root) {
        config_->root_ = root;
        return *this;
    }

    ConfigBuilder& setIndex(const std::string& index) {
        config_->index_ = index;
        return *this;
    }

    ConfigBuilder& setClientMaxBodySize(uint64_t size) {
        config_->client_max_body_size_ = size;
        return *this;
    }

    ConfigBuilder& addErrorPage(uint16_t code, const std::string& page) {
        config_->error_pages_[code] = page;
        return *this;
    }

    // Location configuration methods
    void startLocation(const std::string& path) {
        current_location_ = std::make_shared<Location>(path);
    }

    void setLocationRoot(const std::string& root) {
        if (current_location_) {
            current_location_->root_ = root;
        }
    }

    void setLocationIndex(const std::string& index) {
        if (current_location_) {
            current_location_->index_ = index;
        }
    }

    void setLocationMethods(const std::vector<std::string>& methods) {
        if (current_location_) {
            current_location_->allowed_methods_ = methods;
        }
    }

    void setLocationAutoindex(bool enabled) {
        if (current_location_) {
            current_location_->autoindex_ = enabled;
        }
    }

    void setLocationRedirect(const std::string& redirect) {
        if (current_location_) {
            current_location_->redirect_ = redirect;
        }
    }

    void endLocation() {
        if (current_location_) {
            config_->locations_.push_back(current_location_);
            current_location_.reset();
        }
    }

    // Build method
    std::unique_ptr<Configuration> build() {
        if (current_location_) {
            endLocation(); // Ensure any in-progress location is added
        }
        return std::move(config_);
    }

private:
    std::unique_ptr<Configuration> config_;
    std::shared_ptr<Location> current_location_;
};

#endif