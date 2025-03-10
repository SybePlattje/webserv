#include "Config.hpp"

std::unique_ptr<Config> ConfigParser::parse(std::istream& input) {
    ConfigLexer lexer(input);
    ConfigParser parser(lexer);
    return parser.parseConfig();
}

void ConfigParser::advance() {
    current_token_ = lexer_.nextToken();
    if (current_token_.type == TokenType::INVALID) {
        throw ParseError(lexer_.getError(), current_token_);
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
    Token prev = current_token_;  // Store for error reporting
    if (!match(type)) {
        throw ParseError(error_msg, prev, true);  // Use end position
    }
}

std::string ConfigParser::expectIdentifier(const std::string& error_msg) {
    if (current_token_.type != TokenType::IDENTIFIER) {
        throw ParseError(error_msg, current_token_);
    }
    std::string value = current_token_.value;
    advance();
    return value;
}

std::vector<std::string> ConfigParser::expectIdentifierList(const std::string& error_msg) {
    std::vector<std::string> values;
    
    if (current_token_.type != TokenType::IDENTIFIER && current_token_.type != TokenType::NUMBER) {
        throw ParseError(error_msg, current_token_);
    }

    while (current_token_.type == TokenType::IDENTIFIER || current_token_.type == TokenType::NUMBER) {
        values.push_back(current_token_.value);
        advance();
    }

    return values;
}

uint64_t ConfigParser::expectNumber(const std::string& error_msg) {
    if (current_token_.type != TokenType::NUMBER) {
        throw ParseError(error_msg, current_token_);
    }
    uint64_t value = std::stoull(current_token_.value);
    advance();
    return value;
}

void ConfigParser::parseReturn(ConfigBuilder& builder) {
    // Parse status code
    valueToken = current_token_;  // Store before reading value
    unsigned int code = static_cast<unsigned int>(expectNumber("Expected status code"));

    // Get optional URL or message
    std::string body;
    if (current_token_.type == TokenType::IDENTIFIER ||
        current_token_.type == TokenType::STRING) {
        valueToken = current_token_;  // Update for URL/message token
        body = (current_token_.type == TokenType::STRING) ?
               current_token_.value : expectIdentifier("Expected URL or message");
        
        if (current_token_.type == TokenType::STRING) {
            advance();  // Skip the string token if we consumed it
        }
    }

    // Handle different types of return directives
    if ((code >= 301 && code <= 303) || code == 307 || code == 308) {
        // Redirect requires a URL
        if (body.empty()) {
            throw ParseError("Redirect requires a URL", valueToken);
        }
        builder.setLocationRedirect(code, body);
    } else if (code == 200 || (code >= 400 && code <= 405)) {
        // Response with optional message
        builder.setLocationResponse(code, body);
    } else {
        throw ParseError("Invalid status code for return directive", valueToken);
    }
}

std::unique_ptr<Config> ConfigParser::parseConfig() {
    ConfigBuilder builder;
    advance(); // Get first token

    if (current_token_.value != "server") {
        throw ParseError("Expected 'server' block", current_token_);
    }
    advance();

    parseServerBlock(builder);

    if (current_token_.type != TokenType::END_OF_FILE) {
        throw ParseError("Unexpected content after server block", current_token_);
    }

    return builder.build();
}

void ConfigParser::parseServerBlock(ConfigBuilder& builder) {
    expect(TokenType::LBRACE, "Expected '{' after 'server'");

    while (current_token_.type != TokenType::RBRACE) {
        if (current_token_.type == TokenType::END_OF_FILE) {
            throw ParseError("Unexpected end of file", current_token_);
        }

        if (current_token_.value == "location") {
            advance();
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
            throw ParseError("Unexpected end of file", current_token_);
        }
        parseDirective(builder, true);
    }

    builder.endLocation();
    advance(); // Skip closing brace
}

void ConfigParser::parseDirective(ConfigBuilder& builder, bool in_location) {
    std::string directive = expectIdentifier("Expected directive name");
    
    if (in_location) {
        if (directive == "root") {
            valueToken = current_token_;  // Store before reading value
            std::string root = expectIdentifier("Expected root path");
            builder.setLocationRoot(root);
        } else if (directive == "index") {
            valueToken = current_token_;  // Store before reading value
            auto index_files = expectIdentifierList("Expected index filename");
            if (!index_files.empty()) {
                builder.setLocationIndex(index_files[0]);
            }
        } else if (directive == "allow_methods") {
            valueToken = current_token_;  // Store before reading value
            auto methods = expectIdentifierList("Expected at least one HTTP method");
            if (methods.empty()) {
                throw ParseError("Expected at least one HTTP method", valueToken, true);
            }
            builder.setLocationMethods(methods);
        } else if (directive == "autoindex") {
            valueToken = current_token_;  // Store before reading value
            std::string value = expectIdentifier("Expected 'on' or 'off'");
            if (value != "on" && value != "off") {
                throw ParseError("autoindex value must be 'on' or 'off'", valueToken, true);
            }
            builder.setLocationAutoindex(value == "on");
        } else if (directive == "return") {
            valueToken = current_token_;  // Store before reading value
            parseReturn(builder);
        } else {
            throw ParseError("Unknown location directive: " + directive, valueToken);
        }
    } else {
        if (directive == "listen") {
            valueToken = current_token_;  // Store before reading value
            uint64_t port = expectNumber("Expected port number");
            if (port > 65535) {
                throw ParseError("Port number out of range", valueToken);
            }
            builder.setPort(static_cast<uint16_t>(port));
        } else if (directive == "server_name") {
            valueToken = current_token_;  // Store before reading value
            std::string name = expectIdentifier("Expected server name");
            builder.setServerName(name);
        } else if (directive == "root") {
            valueToken = current_token_;  // Store before reading value
            std::string root = expectIdentifier("Expected root path");
            builder.setRoot(root);
        } else if (directive == "index") {
            valueToken = current_token_;  // Store before reading value
            auto index_files = expectIdentifierList("Expected index filename");
            if (!index_files.empty()) {
                builder.setIndex(index_files[0]);
            }
        } else if (directive == "client_max_body_size") {
            valueToken = current_token_;  // Store before reading value
            uint64_t size = expectNumber("Expected body size");
            builder.setClientMaxBodySize(size);
        } else if (directive == "error_page") {
            valueToken = current_token_;  // Store before reading value
            uint64_t code = expectNumber("Expected error code");
            if (code < 400 || code > 599) {
                throw ParseError("Invalid error code", valueToken);
            }
            valueToken = current_token_;  // Store before reading error page
            std::string page = expectIdentifier("Expected error page path");
            builder.addErrorPage(static_cast<uint16_t>(code), page);
        } else {
            throw ParseError("Unknown server directive: " + directive, valueToken);
        }
    }

    // Use the end position of the last value token when reporting missing semicolon
    if (!match(TokenType::SEMICOLON)) {
        throw ParseError("Expected ';' after directive", valueToken, true);
    }
}