#include "ConfigParser.hpp"
#include <algorithm>
#include <cctype>

namespace {
    // Helper functions
    std::string trim(const std::string& str) {
        const auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
        const auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
        return (start < end ? std::string(start, end) : std::string());
    }

    bool isComment(const std::string& line) {
        const auto first_non_space = std::find_if_not(line.begin(), line.end(), ::isspace);
        return first_non_space != line.end() && *first_non_space == '#';
    }

    // Helper function to get first index file (we'll use the first one as primary)
    std::string getFirstIndex(const std::vector<std::string>& tokens) {
        return tokens.size() > 1 ? tokens[1] : "";
    }
}

void ConfigParser::parseConfiguration(std::istream& stream, ConfigurationBuilder& builder) {
    std::string line;
    size_t line_number = 0;
    bool found_server = false;

    while (std::getline(stream, line)) {
        line_number++;
        line = trim(line);

        if (line.empty() || isComment(line)) {
            continue;
        }

        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "server") {
            std::string brace;
            if (!(iss >> brace) || brace != "{") {
                throw ParserException("Expected { after server", line_number);
            }
            if (found_server) {
                throw ParserException("Multiple server blocks not supported", line_number);
            }
            found_server = true;
            parseServerBlock(stream, builder, line_number);
        } else {
            throw ParserException("Expected server block", line_number);
        }
    }

    if (!found_server) {
        throw ParserException("No server block found", line_number);
    }
}

std::vector<std::string> ConfigParser::tokenizeLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    
    while (iss >> token) {
        if (token.back() == ';') {
            token.pop_back();  // Remove semicolon
            if (!token.empty()) {
                tokens.push_back(token);
            }
            break;  // End of directive
        }
        tokens.push_back(token);
    }
    
    return tokens;
}

void ConfigParser::parseServerBlock(std::istream& stream, ConfigurationBuilder& builder, size_t& line_number) {
    std::string line;
    
    while (std::getline(stream, line)) {
        line_number++;
        line = trim(line);
        
        if (line.empty() || isComment(line)) {
            continue;
        }
        
        if (isBlockEnd(line)) {
            return;
        }
        
        std::string blockType, blockPath;
        if (isBlockStart(line, blockType, blockPath, line_number)) {
            if (blockType == "location") {
                parseLocationBlock(stream, blockPath, builder, line_number);
            } else {
                throw ParserException("Unexpected block: " + blockType, line_number);
            }
        } else {
            auto tokens = tokenizeLine(line);
            if (!tokens.empty()) {
                handleDirective(tokens[0], tokens, builder, line_number);
            }
        }
    }
    
    throw ParserException("Unexpected end of file, missing closing brace", line_number);
}

void ConfigParser::parseLocationBlock(std::istream& stream, const std::string& path,
                                    ConfigurationBuilder& builder, size_t line_number) {
    LocationConfig location(path);
    std::string line;
    
    while (std::getline(stream, line)) {
        line = trim(line);
        
        if (line.empty() || isComment(line)) {
            continue;
        }
        
        if (isBlockEnd(line)) {
            builder.addLocation(location);
            return;
        }
        
        auto tokens = tokenizeLine(line);
        if (!tokens.empty()) {
            const std::string& directive = tokens[0];
            if (directive == "root") {
                validateDirective(directive, tokens, 2, line_number);
                location.setRoot(tokens[1]);
            } else if (directive == "index") {
                if (tokens.size() < 2) {
                    throw ParserException("index requires at least one argument", line_number);
                }
                location.setIndex(getFirstIndex(tokens));
            } else if (directive == "return") {
                validateDirective(directive, tokens, 2, line_number);
                location.setRedirect(tokens[1]);
            } else if (directive == "allow_methods") {
                if (tokens.size() < 2) {
                    throw ParserException("allow_methods requires at least one method", line_number);
                }
                tokens.erase(tokens.begin());  // Remove directive name
                location.setAllowedMethods(tokens);
            } else if (directive == "autoindex") {
                validateDirective(directive, tokens, 2, line_number);
                location.setAutoindex(tokens[1] == "on");
            } else {
                throw ParserException("Unknown location directive: " + directive, line_number);
            }
        }
        line_number++;
    }
    
    throw ParserException("Unexpected end of file in location block", line_number);
}

void ConfigParser::handleDirective(const std::string& directive, 
                                 const std::vector<std::string>& tokens,
                                 ConfigurationBuilder& builder,
                                 size_t line_number) {
    if (directive == "listen") {
        validateDirective(directive, tokens, 2, line_number);
        try {
            builder.setPort(static_cast<uint16_t>(std::stoi(tokens[1])));
        } catch (const std::exception&) {
            throw ParserException("Invalid port number: " + tokens[1], line_number);
        }
    } else if (directive == "server_name") {
        validateDirective(directive, tokens, 2, line_number);
        builder.setServerName(tokens[1]);
    } else if (directive == "root") {
        validateDirective(directive, tokens, 2, line_number);
        builder.setRoot(tokens[1]);
    } else if (directive == "index") {
        if (tokens.size() < 2) {
            throw ParserException("index requires at least one argument", line_number);
        }
        builder.setIndex(getFirstIndex(tokens));
    } else if (directive == "client_max_body_size") {
        validateDirective(directive, tokens, 2, line_number);
        try {
            builder.setClientMaxBodySize(std::stoull(tokens[1]));
        } catch (const std::exception&) {
            throw ParserException("Invalid client_max_body_size: " + tokens[1], line_number);
        }
    } else if (directive == "error_page") {
        if (tokens.size() != 3) {
            throw ParserException("error_page requires exactly 2 arguments", line_number);
        }
        try {
            uint16_t code = static_cast<uint16_t>(std::stoi(tokens[1]));
            builder.addErrorPage(code, tokens[2]);
        } catch (const std::exception&) {
            throw ParserException("Invalid error code: " + tokens[1], line_number);
        }
    } else {
        throw ParserException("Unknown directive: " + directive, line_number);
    }
}

bool ConfigParser::isBlockStart(const std::string& line, std::string& blockType,
                              std::string& blockPath, size_t line_number) {
    std::istringstream iss(line);
    iss >> blockType;
    
    if (blockType == "location") {
        iss >> blockPath;
        std::string brace;
        if (!(iss >> brace) || brace != "{") {
            throw ParserException("Expected { after location path", line_number);
        }
        return true;
    }
    
    return false;
}

bool ConfigParser::isBlockEnd(const std::string& line) {
    return line == "}";
}

void ConfigParser::validateDirective(const std::string& directive,
                                   const std::vector<std::string>& tokens,
                                   size_t expected_tokens,
                                   size_t line_number) {
    if (tokens.size() != expected_tokens) {
        throw ParserException(directive + " requires exactly " + 
                            std::to_string(expected_tokens - 1) + " argument(s)",
                            line_number);
    }
}