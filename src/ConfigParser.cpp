#include "ConfigParser.hpp"

std::unique_ptr<Configuration> ConfigParser::parse(std::istream& input) {
    ConfigLexer lexer(input);
    ConfigParser parser(lexer);
    return parser.parseConfig();
}

void ConfigParser::advance() {
    current_token_ = lexer_.nextToken();
    if (current_token_.type == TokenType::INVALID) {
        throw ParseError(lexer_.getError(), current_token_.line, current_token_.column);
    }
}

bool ConfigParser::match(TokenType type) {
    if (current_token_.type == type) {
        advance();
        return true;
    }
    return false;
}

void ConfigParser::expect(TokenType type, const std::string& error_msg) {
    if (!match(type)) {
        throw ParseError(error_msg, current_token_.line, current_token_.column);
    }
}

std::string ConfigParser::expectIdentifier(const std::string& error_msg) {
    if (current_token_.type != TokenType::IDENTIFIER) {
        throw ParseError(error_msg, current_token_.line, current_token_.column);
    }
    std::string value = current_token_.value;
    advance();
    return value;
}

std::vector<std::string> ConfigParser::expectIdentifierList(const std::string& error_msg) {
    std::vector<std::string> values;
    
    if (current_token_.type != TokenType::IDENTIFIER && current_token_.type != TokenType::NUMBER) {
        throw ParseError(error_msg, current_token_.line, current_token_.column);
    }

    while (current_token_.type == TokenType::IDENTIFIER || current_token_.type == TokenType::NUMBER) {
        values.push_back(current_token_.value);
        advance();
    }

    return values;
}

uint64_t ConfigParser::expectNumber(const std::string& error_msg) {
    if (current_token_.type != TokenType::NUMBER) {
        throw ParseError(error_msg, current_token_.line, current_token_.column);
    }
    uint64_t value = std::stoull(current_token_.value);
    advance();
    return value;
}

std::unique_ptr<Configuration> ConfigParser::parseConfig() {
    ConfigBuilder builder;
    advance(); // Get first token

    // Must start with 'server'
    if (current_token_.value != "server") {
        throw ParseError("Expected 'server' block", current_token_.line, current_token_.column);
    }
    advance();

    parseServerBlock(builder);

    // Make sure we've reached the end
    if (current_token_.type != TokenType::END_OF_FILE) {
        throw ParseError("Unexpected content after server block", 
                        current_token_.line, current_token_.column);
    }

    return builder.build();
}

void ConfigParser::parseServerBlock(ConfigBuilder& builder) {
    expect(TokenType::LBRACE, "Expected '{' after 'server'");

    while (current_token_.type != TokenType::RBRACE) {
        if (current_token_.type == TokenType::END_OF_FILE) {
            throw ParseError("Unexpected end of file", current_token_.line, current_token_.column);
        }

        if (current_token_.value == "location") {
            advance(); // Skip 'location'
            parseLocationBlock(builder);
        } else {
            parseDirective(builder, false);
        }
    }

    advance(); // Skip closing brace
}

void ConfigParser::parseLocationBlock(ConfigBuilder& builder) {
    std::string path = expectIdentifier("Expected location path");
    builder.startLocation(path);

    expect(TokenType::LBRACE, "Expected '{' after location path");

    while (current_token_.type != TokenType::RBRACE) {
        if (current_token_.type == TokenType::END_OF_FILE) {
            throw ParseError("Unexpected end of file", current_token_.line, current_token_.column);
        }
        parseDirective(builder, true);
    }

    builder.endLocation();
    advance(); // Skip closing brace
}

void ConfigParser::parseDirective(ConfigBuilder& builder, bool in_location) {
    std::string directive = expectIdentifier("Expected directive name");
    
    if (in_location) {
        // Location directives
        if (directive == "root") {
            std::string root = expectIdentifier("Expected root path");
            builder.setLocationRoot(root);
        } else if (directive == "index") {
            auto index_files = expectIdentifierList("Expected index filename");
            if (!index_files.empty()) {
                builder.setLocationIndex(index_files[0]); // Use first file as primary index
            }
        } else if (directive == "allow_methods") {
            auto methods = expectIdentifierList("Expected at least one HTTP method");
            if (methods.empty()) {
                throw ParseError("Expected at least one HTTP method", 
                               current_token_.line, current_token_.column);
            }
            builder.setLocationMethods(methods);
        } else if (directive == "autoindex") {
            std::string value = expectIdentifier("Expected 'on' or 'off'");
            if (value != "on" && value != "off") {
                throw ParseError("autoindex value must be 'on' or 'off'", 
                               current_token_.line, current_token_.column);
            }
            builder.setLocationAutoindex(value == "on");
        } else if (directive == "return") {
            std::string redirect = expectIdentifier("Expected redirect path");
            builder.setLocationRedirect(redirect);
        } else {
            throw ParseError("Unknown location directive: " + directive, 
                           current_token_.line, current_token_.column);
        }
    } else {
        // Server directives
        if (directive == "listen") {
            uint64_t port = expectNumber("Expected port number");
            if (port > 65535) {
                throw ParseError("Port number out of range", current_token_.line, current_token_.column);
            }
            builder.setPort(static_cast<uint16_t>(port));
        } else if (directive == "server_name") {
            std::string name = expectIdentifier("Expected server name");
            builder.setServerName(name);
        } else if (directive == "root") {
            std::string root = expectIdentifier("Expected root path");
            builder.setRoot(root);
        } else if (directive == "index") {
            auto index_files = expectIdentifierList("Expected index filename");
            if (!index_files.empty()) {
                builder.setIndex(index_files[0]); // Use first file as primary index
            }
        } else if (directive == "client_max_body_size") {
            uint64_t size = expectNumber("Expected body size");
            builder.setClientMaxBodySize(size);
        } else if (directive == "error_page") {
            uint64_t code = expectNumber("Expected error code");
            if (code < 400 || code > 599) {
                throw ParseError("Invalid error code", current_token_.line, current_token_.column);
            }
            std::string page = expectIdentifier("Expected error page path");
            builder.addErrorPage(static_cast<uint16_t>(code), page);
        } else {
            throw ParseError("Unknown server directive: " + directive, 
                           current_token_.line, current_token_.column);
        }
    }

    expect(TokenType::SEMICOLON, "Expected ';' after directive");
}