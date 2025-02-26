#include "old/Location.hpp"

Location::Location(const std::string& path) : path_(path), autoindex_(false) {}

Location::~Location() {}

Location::Location(const Location& other)
{
    *this = other;
}

Location& Location::operator=(const Location& other)
{
    if (this != &other)
    {
        path_ = other.path_;
        root_ = other.root_;
        index_ = other.index_;
        allowed_methods_ = other.allowed_methods_;
        autoindex_ = other.autoindex_;
    }
    return *this;
}

void Location::parse(std::istream& stream)
{
    std::string line;

    while (getline(stream, line))
    {
        if (ConfigUtils::isCommentOrEmpty(line))
            continue;

        if (ConfigUtils::isEndOfBlock(line))
            break;

        std::vector<std::string> tokens = ConfigUtils::splitIntoTokens(line);
        if (tokens.empty())
            continue;

        // Validate line syntax
        ConfigUtils::validateLineSyntax(line, tokens);

        const std::string& key = tokens[0];
        if (key == "root")
            root_ = tokens[1];
        else if (key == "index")
            index_ = tokens[1];
        else if (key == "allow_methods")
        {
            for (size_t i = 1; i < tokens.size(); ++i)
                allowed_methods_.push_back(tokens[i]);
        }
        else if (key == "autoindex")
            autoindex_ = (tokens[1] == "on");
        else if (key == "return")
            return_ = tokens[1];
        else
            throw std::runtime_error("Config: Unsupported directive in location block: " + key);
    }
}

void Location::printLocation() const
{
    std::cout << "Location Path: " << path_ << "\n"
              << "Root: " << root_ << "\n"
              << "Index: " << index_ << "\n"
              << "Allowed Methods: ";
    for (const auto& method : allowed_methods_)
        std::cout << method << " ";
    std::cout << "\nAutoindex: " << (autoindex_ ? "on" : "off") << std::endl;
}

const std::string& Location::getPath() const
{
    return path_;
}

const std::string& Location::getRoot() const
{
    return root_;
}

const std::string& Location::getIndex() const
{
    return index_;
}

const std::string& Location::getReturn() const
{
    return return_;
}

const std::vector<std::string>& Location::getAllowedMethods() const
{
    return allowed_methods_;
}

bool Location::getAutoindex() const
{
    return autoindex_;
}
