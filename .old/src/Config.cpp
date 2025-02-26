#include "old/Config.hpp"

Config::Config(const char *argv)
    : listen_(9999), server_name_("localhost"), root_("/"), client_max_body_size_(0),
      has_listen_(false), has_server_name_(false), has_root_(false), 
      has_index_(false), has_client_max_body_size_(false)
{
    const std::string path_to_config = argv ? argv : DEFAULT_CONFIG;
    parseConfigFile(path_to_config);
}

void Config::parseConfigFile(const std::string& path_to_config) {
    std::ifstream configFile(path_to_config);
    if (!configFile.is_open())
        throw std::runtime_error("Config: Unable to open configuration file: " + path_to_config);

    std::string line;
    bool foundServerBlock = false;

    while (getline(configFile, line))
        handleConfigLine(line, foundServerBlock, configFile);

    if (!foundServerBlock)
        throw std::runtime_error("Config: No server block found in configuration file");

    validateRequiredDirectives();
    configFile.close();
}

void Config::handleConfigLine(const std::string& line, bool& foundServerBlock, std::istream& stream) {
    if (ConfigUtils::isCommentOrEmpty(line))
        return;

    if (line.find("{") != std::string::npos)
        handleBlockStart(line, foundServerBlock, stream);
    else if (line.find("}") != std::string::npos)
        throw std::runtime_error("Config: Unexpected closing brace outside of block");
    else if (!ConfigUtils::isCommentOrEmpty(line))
        throw std::runtime_error("Config: Directives must be inside server block");
}

void Config::handleBlockStart(const std::string& line, bool& foundServerBlock, std::istream& stream) {
    validateBlockStructure(line, true);
    std::string blockType = ConfigUtils::extractBlockType(line);
    
    if (blockType == "server") {
        if (foundServerBlock)
            throw std::runtime_error("Config: Multiple server blocks are not supported yet");
        foundServerBlock = true;
        parseServerBlock(stream);
    }
    else if (blockType == "location")
        throw std::runtime_error("Config: Location blocks must be inside server block");
    else
        throw std::runtime_error("Config: Unsupported block type: " + blockType);
}

void Config::validateBlockStructure(const std::string& line, bool isOpening) {
    if (isOpening) {
        if (line.find("}") != std::string::npos)
            throw std::runtime_error("Config: Invalid block format - opening and closing brace on same line");
    } else {
        if (line.find("{") != std::string::npos)
            throw std::runtime_error("Config: Invalid block format - opening and closing brace on same line");
    }
}

void Config::parseServerBlock(std::istream& stream) {
    std::string line;
    bool hasClosingBrace = false;

    while (getline(stream, line)) {
        if (ConfigUtils::isCommentOrEmpty(line))
            continue;

        if (line.find("}") != std::string::npos) {
            validateBlockStructure(line, false);
            hasClosingBrace = true;
            break;
        }

        std::vector<std::string> tokens = ConfigUtils::splitIntoTokens(line);
        if (tokens.empty())
            continue;

        if (ConfigUtils::needsSemicolon(line))
            ConfigUtils::validateLineSyntax(line, tokens);

        handleServerDirective(line, tokens, stream);
    }

    if (!hasClosingBrace)
        throw std::runtime_error("Config: Missing closing brace for server block");
}

void Config::handleServerDirective(const std::string& line, const std::vector<std::string>& tokens, std::istream& stream) {
    const std::string& directive = tokens[0];
    try {
        if (line.find("{") != std::string::npos) {
            if (directive != "location")
                throw std::runtime_error("Config: Unexpected block in server context: " + directive);
            if (tokens.size() < 2)
                throw std::runtime_error("Invalid number of arguments for location directive");
            validatePath(tokens[1]);
            parseLocationBlock(stream, tokens[1]);
        }
        else if (directive == "listen")
            handleListenDirective(tokens);
        else if (directive == "server_name")
            handleServerNameDirective(tokens);
        else if (directive == "root")
            handleRootDirective(tokens);
        else if (directive == "index")
            handleIndexDirective(tokens);
        else if (directive == "error_page")
            handleErrorPageDirective(tokens);
        else if (directive == "client_max_body_size")
            handleClientMaxBodySizeDirective(tokens);
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

// Directive handlers
void Config::handleListenDirective(const std::vector<std::string>& tokens) {
    validateArgs("listen", tokens, 2);
    checkDirectiveNotSet("listen", has_listen_);
    listen_ = std::stoi(tokens[1]);
    validatePort(listen_);
    has_listen_ = true;
}

void Config::handleServerNameDirective(const std::vector<std::string>& tokens) {
    validateArgs("server_name", tokens, 2);
    checkDirectiveNotSet("server_name", has_server_name_);
    server_name_ = tokens[1];
    has_server_name_ = true;
}

void Config::handleRootDirective(const std::vector<std::string>& tokens) {
    validateArgs("root", tokens, 2);
    checkDirectiveNotSet("root", has_root_);
    validatePath(tokens[1]);
    root_ = tokens[1];
    has_root_ = true;
}

void Config::handleIndexDirective(const std::vector<std::string>& tokens) {
    validateArgs("index", tokens, 2);
    checkDirectiveNotSet("index", has_index_);
    index_ = tokens[1];
    has_index_ = true;
}

void Config::handleErrorPageDirective(const std::vector<std::string>& tokens) {
    validateArgs("error_page", tokens, 3);
    validateErrorCode(tokens[1]);
    validatePath(tokens[2]);
    error_pages_[std::stoi(tokens[1])] = tokens[2];
}

void Config::handleClientMaxBodySizeDirective(const std::vector<std::string>& tokens) {
    validateArgs("client_max_body_size", tokens, 2);
    checkDirectiveNotSet("client_max_body_size", has_client_max_body_size_);
    client_max_body_size_ = std::stoi(tokens[1]);
    has_client_max_body_size_ = true;
}

// Validation methods
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

void Config::validateErrorCode(const std::string& code) {
    uint errorCode = std::stoi(code);
    if (errorCode < 400 || errorCode > 599)
        throw std::runtime_error("Invalid error code: " + code);
}

void Config::validateRequiredDirectives() const {
    if (!has_listen_)
        throw std::runtime_error("Config: Missing required directive: listen");
    if (!has_server_name_)
        throw std::runtime_error("Config: Missing required directive: server_name");
}

void Config::validateArgs(const std::string& directive, const std::vector<std::string>& tokens, size_t expected) {
    if (tokens.size() != expected)
        throw std::runtime_error("Invalid number of arguments for " + directive + " directive");
}

void Config::checkDirectiveNotSet(const std::string& directive, bool has_directive) const {
    if (has_directive)
        throw std::runtime_error("Config: Duplicate directive found: " + directive);
}

// Location block handling
void Config::parseLocationBlock(std::istream& stream, const std::string& path) {
    Location loc(path);
    loc.parse(stream);
    locations_.push_back(loc);
}

// Utility methods
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

// Debug output
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

// Getters
const uint& Config::getListen() const { return listen_; }
const std::string& Config::getServerName() const { return server_name_; }
const std::string& Config::getRoot() const { return root_; }
const std::string& Config::getIndex() const { return index_; }
const std::map<uint, std::string>& Config::getErrorPages() const { return error_pages_; }
const uint& Config::getClientMaxBodySize() const { return client_max_body_size_; }
const std::vector<Location>& Config::getLocations() const { return locations_; }
