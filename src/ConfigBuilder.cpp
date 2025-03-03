#include "ConfigBuilder.hpp"

ConfigBuilder::ConfigBuilder() : config_(new Config()) {}

ConfigBuilder& ConfigBuilder::setPort(uint16_t port) {
    config_->port_ = port;
    return *this;
}

ConfigBuilder& ConfigBuilder::setServerName(const std::string& name) {
    config_->server_name_ = name;
    return *this;
}

ConfigBuilder& ConfigBuilder::setRoot(const std::string& root) {
    config_->root_ = root;
    return *this;
}

ConfigBuilder& ConfigBuilder::setIndex(const std::string& index) {
    config_->index_ = index;
    return *this;
}

ConfigBuilder& ConfigBuilder::setClientMaxBodySize(uint64_t size) {
    config_->client_max_body_size_ = size;
    return *this;
}

ConfigBuilder& ConfigBuilder::addErrorPage(uint16_t code, const std::string& page) {
    config_->error_pages_[code] = page;
    return *this;
}

void ConfigBuilder::startLocation(const std::string& path) {
    current_location_ = std::make_shared<Location>(path);
}

void ConfigBuilder::setLocationRoot(const std::string& root) {
    if (!current_location_) {
        throw std::runtime_error("No active location block");
    }
    current_location_->root_ = root;
}

void ConfigBuilder::setLocationIndex(const std::string& index) {
    if (!current_location_) {
        throw std::runtime_error("No active location block");
    }
    current_location_->index_ = index;
}

void ConfigBuilder::setLocationMethods(const std::vector<std::string>& methods) {
    if (!current_location_) {
        throw std::runtime_error("No active location block");
    }
    current_location_->allowed_methods_ = methods;
}

void ConfigBuilder::setLocationAutoindex(bool enabled) {
    if (!current_location_) {
        throw std::runtime_error("No active location block");
    }
    current_location_->autoindex_ = enabled;
}

void ConfigBuilder::setLocationRedirect(unsigned int code, const std::string& url) {
    if (!current_location_) {
        throw std::runtime_error("No active location block");
    }
    if (!Location::isValidRedirectCode(code)) {
        throw std::runtime_error("Invalid redirect status code: " + std::to_string(code));
    }
    current_location_->return_directive_ = Location::ReturnDirective(
        Location::ReturnType::REDIRECT, code, url);
}

void ConfigBuilder::setLocationResponse(unsigned int code, const std::string& message) {
    if (!current_location_) {
        throw std::runtime_error("No active location block");
    }
    if (!Location::isValidResponseCode(code)) {
        throw std::runtime_error("Invalid response status code: " + std::to_string(code));
    }
    current_location_->return_directive_ = Location::ReturnDirective(
        Location::ReturnType::RESPONSE, code, message);
}

void ConfigBuilder::endLocation() {
    if (current_location_) {
        config_->locations_.push_back(current_location_);
        current_location_.reset();
    }
}

std::unique_ptr<Config> ConfigBuilder::build() {
    if (current_location_) {
        endLocation(); // Ensure any in-progress location is added
    }
    return std::move(config_);
}