#ifndef CONFIGURATION_BUILDER_HPP
#define CONFIGURATION_BUILDER_HPP

#include <memory>
#include "Configuration.hpp"
#include "LocationConfig.hpp"

class ConfigurationBuilder {
public:
    ConfigurationBuilder() : config_(new Configuration()) {}

    ConfigurationBuilder& setPort(uint16_t port) {
        config_->port_ = port;
        return *this;
    }

    ConfigurationBuilder& setServerName(const std::string& name) {
        config_->server_name_ = name;
        return *this;
    }

    ConfigurationBuilder& setRoot(const std::string& root) {
        config_->root_ = root;
        return *this;
    }

    ConfigurationBuilder& setIndex(const std::string& index) {
        config_->index_ = index;
        return *this;
    }

    ConfigurationBuilder& setClientMaxBodySize(uint64_t size) {
        config_->client_max_body_size_ = size;
        return *this;
    }

    ConfigurationBuilder& addErrorPage(uint16_t code, const std::string& page) {
        config_->error_pages_[code] = page;
        return *this;
    }

    ConfigurationBuilder& addLocation(const LocationConfig& location) {
        config_->locations_.push_back(location);
        return *this;
    }

    std::unique_ptr<IConfiguration> build() {
        return std::move(config_);
    }

private:
    std::unique_ptr<Configuration> config_;
};

#endif