#include "Config.hpp"

Config::Config(const char *argv)
    : listen_(9999), server_name_("localhost"), root_("/"), client_max_body_size_(0),
      has_listen_(false), has_server_name_(false), has_root_(false), 
      has_index_(false), has_client_max_body_size_(false)
{
    std::string line;
    const std::string path_to_config = argv ? argv : DEFAULT_CONFIG;
    std::ifstream configFile(path_to_config);

    if (!configFile.is_open())
        throw std::runtime_error("Config: Unable to open configuration file: " + path_to_config);

    bool foundServerBlock = false;
    while (getline(configFile, line)) {
        if (ConfigUtils::isCommentOrEmpty(line))
            continue;
        
        if (line.find("{") != std::string::npos) {
            std::string blockType = ConfigUtils::extractBlockType(line);
            if (blockType == "server") {
                if (foundServerBlock)
                    throw std::runtime_error("Config: Multiple server blocks are not supported yet");
                foundServerBlock = true;
                parseServerBlock(configFile);
            }
            else
                throw std::runtime_error("Config: Unsupported block type: " + blockType);
        }
    }
    
    if (!foundServerBlock)
        throw std::runtime_error("Config: No server block found in configuration file");

    validateRequiredDirectives();
    configFile.close();
}

Config::~Config() {}

Config::Config(const Config& other) {
    *this = other;
}

Config& Config::operator=(const Config& other) {
    if (this != &other) {
        server_name_ = other.server_name_;
        listen_ = other.listen_;
        root_ = other.root_;
        index_ = other.index_;
        error_pages_ = other.error_pages_;
        client_max_body_size_ = other.client_max_body_size_;
        locations_ = other.locations_;
        
        has_listen_ = other.has_listen_;
        has_server_name_ = other.has_server_name_;
        has_root_ = other.has_root_;
        has_index_ = other.has_index_;
        has_client_max_body_size_ = other.has_client_max_body_size_;
    }
    return *this;
}

void Config::validatePort(const uint port) {
    if (port > 65535)
        throw std::runtime_error("Config: Invalid port number: " + std::to_string(port));
}

void Config::validatePath(const std::string& path) {
    if (path.empty())
        throw std::runtime_error("Config: Empty path is not allowed");
    if (path[0] != '/' && path[0] != '.')
        throw std::runtime_error("Config: Path must start with '/' or '.': " + path);
}

void Config::validateRequiredDirectives() const {
    if (!has_listen_)
        throw std::runtime_error("Config: Missing required directive: listen");
    if (!has_server_name_)
        throw std::runtime_error("Config: Missing required directive: server_name");
}

void Config::checkDirectiveNotSet(const std::string& directive, bool has_directive) const {
    if (has_directive)
        throw std::runtime_error("Config: Duplicate directive found: " + directive);
}

void Config::validateArgs(const std::string& directive, const std::vector<std::string>& tokens, size_t expected) {
    if (tokens.size() != expected)
        throw std::runtime_error("Invalid number of arguments for " + directive + " directive");
}

void Config::parseServerBlock(std::istream& stream) {
    std::string line;

    while (getline(stream, line)) {
        if (ConfigUtils::isEndOfBlock(line))
            break;

        std::vector<std::string> tokens = ConfigUtils::tokenize(line);
        if (tokens.empty())
            continue;

        const std::string& directive = tokens[0];
        try {
            if (directive == "listen") {
                validateArgs(directive, tokens, 2);
                checkDirectiveNotSet(directive, has_listen_);
                listen_ = std::stoi(tokens[1]);
                validatePort(listen_);
                has_listen_ = true;
            }
            else if (directive == "server_name") {
                validateArgs(directive, tokens, 2);
                checkDirectiveNotSet(directive, has_server_name_);
                server_name_ = tokens[1];
                has_server_name_ = true;
            }
            else if (directive == "root") {
                validateArgs(directive, tokens, 2);
                checkDirectiveNotSet(directive, has_root_);
                validatePath(tokens[1]);
                root_ = tokens[1];
                has_root_ = true;
            }
            else if (directive == "index") {
                validateArgs(directive, tokens, 2);
                checkDirectiveNotSet(directive, has_index_);
                index_ = tokens[1];
                has_index_ = true;
            }
            else if (directive == "error_page") {
                validateArgs(directive, tokens, 3);
                uint errorCode = std::stoi(tokens[1]);
                if (errorCode < 400 || errorCode > 599)
                    throw std::runtime_error("Invalid error code: " + tokens[1]);
                validatePath(tokens[2]);
                error_pages_[errorCode] = tokens[2];
            }
            else if (directive == "location") {
                if (tokens.size() < 2)
                    throw std::runtime_error("Invalid number of arguments for location directive");
                validatePath(tokens[1]);
                if (line.find("{") == std::string::npos)
                    throw std::runtime_error("Missing opening brace for location block");
                parseLocationBlock(stream, tokens[1]);
            }
            else if (directive == "client_max_body_size") {
                validateArgs(directive, tokens, 2);
                checkDirectiveNotSet(directive, has_client_max_body_size_);
                client_max_body_size_ = std::stoi(tokens[1]);
                has_client_max_body_size_ = true;
            }
            else
                throw std::runtime_error("Unknown directive: " + directive);
        }
        catch (const std::invalid_argument&) {
            throw std::runtime_error("Config: Invalid value for directive " + directive);
        }
        catch (const std::out_of_range&) {
            throw std::runtime_error("Config: Value out of range for directive " + directive);
        }
    }
}

void Config::parseLocationBlock(std::istream& stream, const std::string& path) {
    Location loc(path);
    loc.parse(stream);
    locations_.push_back(loc);
}

void Config::printConfig() const {
    std::cout << "Server Configuration:\n"
              << "Server Name: " << server_name_ << "\n"
              << "Listen Port: " << listen_ << "\n"
              << "Root: " << root_ << "\n"
              << "Index: " << index_ << "\n"
              << "Error Pages:\n";
    for (const auto& entry : error_pages_)
        std::cout << "  Code " << entry.first << ": " << entry.second << "\n";
    std::cout << "Client Max Body Size: " << client_max_body_size_ << "\n"
              << "Number of Locations: " << locations_.size() << std::endl;
}

const uint& Config::getListen() const { return listen_; }
const std::string& Config::getServerName() const { return server_name_; }
const std::string& Config::getRoot() const { return root_; }
const std::string& Config::getIndex() const { return index_; }
const std::map<uint, std::string>& Config::getErrorPages() const { return error_pages_; }
const uint& Config::getClientMaxBodySize() const { return client_max_body_size_; }
const std::vector<Location>& Config::getLocations() const { return locations_; }
