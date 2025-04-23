#include "Config.hpp"

ConfigBuilder::ConfigBuilder() : config_(std::make_shared<Config>()) {}

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

void ConfigBuilder::startLocation(const std::string& path, Location::MatchType type) {
    current_location_ = std::make_shared<Location>(path, type);
    current_location_->index_ = config_.get()->getIndex();
}

void ConfigBuilder::setLocationRoot(const std::string& root) {
    ensureLocationContext("setLocationRoot");
    current_location_->root_ = root;
}

void ConfigBuilder::setLocationIndex(const std::string& index) {
    ensureLocationContext("setLocationIndex");
    current_location_->index_ = index;
}

void ConfigBuilder::setLocationMethods(const std::vector<std::string>& methods) {
    ensureLocationContext("setLocationMethods");
    current_location_->allowed_methods_ = methods;
}

void ConfigBuilder::setLocationAutoindex(bool enabled) {
    ensureLocationContext("setLocationAutoindex");
    current_location_->autoindex_ = enabled;
}

void ConfigBuilder::setLocationRedirect(unsigned int code, const std::string& url) {
    ensureLocationContext("setLocationRedirect");
    if (!Location::isValidRedirectCode(code)) {
        throw std::runtime_error("Invalid redirect status code: " + std::to_string(code));
    }
    current_location_->return_directive_ = Location::ReturnDirective(
        Location::ReturnType::REDIRECT, code, url);
}

void ConfigBuilder::setLocationResponse(unsigned int code, const std::string& message) {
    ensureLocationContext("setLocationResponse");
    if (!Location::isValidResponseCode(code)) {
        throw std::runtime_error("Invalid response status code: " + std::to_string(code));
    }
    current_location_->return_directive_ = Location::ReturnDirective(
        Location::ReturnType::RESPONSE, code, message);
}

void ConfigBuilder::setLocationCGIPath(const std::vector<std::string>& interpreters) {
    ensureLocationContext("setLocationCGIPath");
    current_location_->cgi_config_.interpreters = interpreters;
}

void ConfigBuilder::setLocationCGIExt(const std::vector<std::string>& extensions) {
    ensureLocationContext("setLocationCGIExt");
    current_location_->cgi_config_.extensions = extensions;
}

void ConfigBuilder::endLocation() {
    if (current_location_) {
        config_->locations_.push_back(current_location_);
        current_location_.reset();
    }
}

std::shared_ptr<Config> ConfigBuilder::build() {
    if (current_location_) {
        endLocation(); // Ensure any in-progress location is added
    }
    return config_;
}