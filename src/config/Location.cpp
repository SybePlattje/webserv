#include "Location.hpp"

Location::Location(const std::string& path) : path_(path) {}

Location::Location(const Location& other)
    : path_(other.path_)
    , root_(other.root_)
    , index_(other.index_)
    , allowed_methods_(other.allowed_methods_)
    , autoindex_(other.autoindex_)
    , return_directive_(other.return_directive_)
{}

Location& Location::operator=(const Location& other) {
    if (this != &other) {
        path_ = other.path_;
        root_ = other.root_;
        index_ = other.index_;
        allowed_methods_ = other.allowed_methods_;
        autoindex_ = other.autoindex_;
        return_directive_ = other.return_directive_;
    }
    return *this;
}

const std::string& Location::getPath() const {
    return path_;
}

const std::string& Location::getRoot() const {
    return root_;
}

const std::string& Location::getIndex() const {
    return index_;
}

const std::vector<std::string>& Location::getAllowedMethods() const {
    return allowed_methods_;
}

bool Location::getAutoindex() const {
    return autoindex_;
}

const Location::ReturnDirective& Location::getReturn() const {
    return return_directive_;
}

bool Location::hasReturn() const {
    return return_directive_.type != ReturnType::NONE;
}

bool Location::hasRedirect() const {
    return return_directive_.isRedirect();
}

bool Location::hasResponse() const {
    return return_directive_.isResponse();
}