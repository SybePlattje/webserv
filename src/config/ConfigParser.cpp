#include "Config.hpp"

// Main entry point
std::unique_ptr<Config> ConfigParser::parse(std::istream& input) {
    ConfigLexer lexer(input);
    ConfigParser parser(lexer);
    return parser.parseConfig();
}

// Main parsing methods
std::unique_ptr<Config> ConfigParser::parseConfig() {
    ConfigBuilder builder;
    advance();

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

    advance();
}

void ConfigParser::parseDirective(ConfigBuilder& builder, bool in_location) {
    std::string directive = expectIdentifier("Expected directive name");
    if (in_location) {
        parseLocationDirective(builder, directive);
    } else {
        parseServerDirective(builder, directive);
    }
}

// Server directive parsing
void ConfigParser::parseServerDirective(ConfigBuilder& builder, const std::string& directive) {
    if (directive == "listen") {
        parseServerListen(builder);
    } else if (directive == "server_name") {
        parseServerName(builder);
    } else if (directive == "root") {
        parseServerRoot(builder);
    } else if (directive == "index") {
        parseServerIndex(builder);
    } else if (directive == "client_max_body_size") {
        parseServerBodySize(builder);
    } else if (directive == "error_page") {
        parseServerErrorPage(builder);
    } else {
        throw ParseError("Unknown server directive: " + directive, current_token_);
    }
}

void ConfigParser::parseServerListen(ConfigBuilder& builder) {
    uint64_t port = readNumber("Expected port number");
    if (port > 65535) {
        throw ParseError("Port number out of range", valueToken);
    }
    builder.setPort(static_cast<uint16_t>(port));
    expectSemicolon();
}

void ConfigParser::parseServerName(ConfigBuilder& builder) {
    handleDirective("server_name", builder,
        [](ConfigBuilder& b, const std::string& value) { b.setServerName(value); });
}

void ConfigParser::parseServerRoot(ConfigBuilder& builder) {
    handleDirective("root", builder,
        [](ConfigBuilder& b, const std::string& value) { b.setRoot(value); });
}

void ConfigParser::parseServerIndex(ConfigBuilder& builder) {
    auto files = readValueList("Expected index filename");
    if (!files.empty()) {
        builder.setIndex(files[0]);
    }
    expectSemicolon();
}

void ConfigParser::parseServerBodySize(ConfigBuilder& builder) {
    uint64_t size = readNumber("Expected body size");
    builder.setClientMaxBodySize(size);
    expectSemicolon();
}

void ConfigParser::parseServerErrorPage(ConfigBuilder& builder) {
    uint64_t code = readNumber("Expected error code");
    if (code < 400 || code > 599) {
        throw ParseError("Invalid error code", valueToken);
    }
    std::string page = readValue("Expected error page path");
    builder.addErrorPage(static_cast<uint16_t>(code), page);
    expectSemicolon();
}

// Location block parsing
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
    advance();
}

void ConfigParser::parseLocationDirective(ConfigBuilder& builder, const std::string& directive) {
    if (directive == "root") {
        parseLocationRoot(builder);
    } else if (directive == "index") {
        parseLocationIndex(builder);
    } else if (directive == "allow_methods") {
        parseLocationMethods(builder);
    } else if (directive == "autoindex") {
        parseLocationAutoindex(builder);
    } else if (directive == "return") {
        parseLocationReturn(builder);
    } else {
        throw ParseError("Unknown location directive: " + directive, current_token_);
    }
}

void ConfigParser::parseLocationRoot(ConfigBuilder& builder) {
    handleDirective("root", builder,
        [](ConfigBuilder& b, const std::string& value) { b.setLocationRoot(value); });
}

void ConfigParser::parseLocationIndex(ConfigBuilder& builder) {
    auto files = readValueList("Expected index filename");
    if (!files.empty()) {
        builder.setLocationIndex(files[0]);
    }
    expectSemicolon();
}

void ConfigParser::parseLocationMethods(ConfigBuilder& builder) {
    auto methods = readValueList("Expected at least one HTTP method");
    if (methods.empty()) {
        throw ParseError("Expected at least one HTTP method", valueToken, true);
    }
    builder.setLocationMethods(methods);
    expectSemicolon();
}

void ConfigParser::parseLocationAutoindex(ConfigBuilder& builder) {
    handleDirective("autoindex", builder,
        [](ConfigBuilder& b, const std::string& value) { b.setLocationAutoindex(value == "on"); },
        [](const Token& token) {
            if (token.value != "on" && token.value != "off") {
                throw ParseError("autoindex value must be 'on' or 'off'", token, true);
            }
        });
}

void ConfigParser::parseLocationReturn(ConfigBuilder& builder) {
    unsigned int code = static_cast<unsigned int>(readNumber("Expected status code"));
    std::string body;

    if (current_token_.type == TokenType::IDENTIFIER ||
        current_token_.type == TokenType::STRING) {
        valueToken = current_token_;
        body = (current_token_.type == TokenType::STRING) ?
               current_token_.value : expectIdentifier("Expected URL or message");
        
        if (current_token_.type == TokenType::STRING) {
            advance();
        }
    }

    if ((code >= 301 && code <= 303) || code == 307 || code == 308) {
        if (body.empty()) {
            throw ParseError("Redirect requires a URL", valueToken);
        }
        builder.setLocationRedirect(code, body);
    } else if (code == 200 || (code >= 400 && code <= 405)) {
        builder.setLocationResponse(code, body);
    } else {
        throw ParseError("Invalid status code for return directive", valueToken);
    }
    expectSemicolon();
}

// Token handling helpers
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
    Token prev = current_token_;
    if (!match(type)) {
        throw ParseError(error_msg, prev, true);
    }
}

void ConfigParser::expectSemicolon() {
    if (!match(TokenType::SEMICOLON)) {
        throw ParseError("Expected ';' after directive", valueToken, true);
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

// Value reading utilities
std::string ConfigParser::readValue(const std::string& error_msg) {
    valueToken = current_token_;
    return expectIdentifier(error_msg);
}

std::vector<std::string> ConfigParser::readValueList(const std::string& error_msg) {
    valueToken = current_token_;
    return expectIdentifierList(error_msg);
}

uint64_t ConfigParser::readNumber(const std::string& error_msg) {
    valueToken = current_token_;
    return expectNumber(error_msg);
}

void ConfigParser::handleDirective(const std::string& directive,
                                 ConfigBuilder& builder,
                                 const DirectiveHandler& handler,
                                 const ValueValidator& validator) {
    std::string value = readValue("Expected value for " + directive);
    if (validator) {
        validator(valueToken);
    }
    handler(builder, value);
    expectSemicolon();
}