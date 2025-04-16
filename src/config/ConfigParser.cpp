#include "Config.hpp"

std::vector<std::unique_ptr<Config>> ConfigParser::parse(std::istream& input) {
    ConfigLexer lexer(input);
    ConfigParser parser(lexer);
    return parser.parseConfigs();
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

std::vector<std::unique_ptr<Config>> ConfigParser::parseConfigs() {
    std::vector<std::unique_ptr<Config>> configs;
    advance();

    while (current_token_.type != TokenType::END_OF_FILE) {
        if (current_token_.value != "server") {
            throw ParseError("Expected 'server' block", current_token_);
        }
        advance();

        configs.push_back(parseServerBlock());
    }

    if (configs.empty()) {
        throw ParseError("No server blocks found in configuration", current_token_);
    }

    return configs;
}

std::unique_ptr<Config> ConfigParser::parseServerBlock() {
    ConfigBuilder builder;
    parseServerBlockContent(builder);
    return builder.build();
}

void ConfigParser::parseServerBlockContent(ConfigBuilder& builder) {
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

void ConfigParser::parseLocationBlock(ConfigBuilder& builder) {
    Location::MatchType match_type = Location::MatchType::PREFIX; // Default to prefix match
    std::string path;

    // Check for location match modifiers
    if (current_token_.type == TokenType::MODIFIER) {
        if (current_token_.value == "=") {
            match_type = Location::MatchType::EXACT;
        } else if (current_token_.value == "^~") {
            match_type = Location::MatchType::PREFERENTIAL_PREFIX;
        } else if (current_token_.value == "~") {
            match_type = Location::MatchType::REGEX;
        } else if (current_token_.value == "~*") {
            match_type = Location::MatchType::REGEX_INSENSITIVE;
        } else {
            throw ParseError("Invalid location modifier: " + current_token_.value, current_token_);
        }
        advance();
    }

    path = expectIdentifier("Expected location path");
    builder.startLocation(path, match_type);

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

void ConfigParser::parseDirective(ConfigBuilder& builder, bool in_location) {
    std::string directive = expectIdentifier("Expected directive name");
    if (in_location) {
        parseLocationDirective(builder, directive);
    } else {
        parseServerDirective(builder, directive);
    }
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
    } else if (directive == "cgi_path") {
        parseLocationCGIPath(builder);
    } else if (directive == "cgi_ext") {
        parseLocationCGIExt(builder);
    } else {
        throw ParseError("Unknown location directive: " + directive, current_token_);
    }
}

void ConfigParser::parseLocationRoot(ConfigBuilder& builder) {
    handleDirective("root", builder,
        [](ConfigBuilder& b, const std::string& value) { b.setLocationRoot(value); });
}

void ConfigParser::parseLocationIndex(ConfigBuilder& builder) {
    handleDirective("index", builder,
        [](ConfigBuilder& b, const std::string& value) { b.setLocationIndex(value); });
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

void ConfigParser::parseLocationCGIPath(ConfigBuilder& builder) {
    auto interpreters = readValueList("Expected CGI interpreter path(s)");
    if (interpreters.empty()) {
        throw ParseError("Expected at least one CGI interpreter path", valueToken, true);
    }
    builder.setLocationCGIPath(interpreters);
    expectSemicolon();
}

void ConfigParser::parseLocationCGIExt(ConfigBuilder& builder) {
    auto extensions = readValueList("Expected CGI file extension(s)");
    if (extensions.empty()) {
        throw ParseError("Expected at least one CGI file extension", valueToken, true);
    }
    // Add dots to extensions if not present
    std::vector<std::string> dotExtensions;
    for (const auto& ext : extensions) {
        dotExtensions.push_back(ext[0] == '.' ? ext : "." + ext);
    }
    builder.setLocationCGIExt(dotExtensions);
    expectSemicolon();
}

void ConfigParser::parseServerDirective(ConfigBuilder& builder, const std::string& directive) {
    if (directive == "listen") {
        uint64_t port = readNumber("Expected port number");
        if (port > 65535) {
            throw ParseError("Port number out of range", valueToken);
        }
        builder.setPort(static_cast<uint16_t>(port));
        expectSemicolon();
    } else if (directive == "server_name") {
        handleDirective(directive, builder,
            [](ConfigBuilder& b, const std::string& value) { b.setServerName(value); });
    } else if (directive == "root") {
        handleDirective(directive, builder,
            [](ConfigBuilder& b, const std::string& value) { b.setRoot(value); });
    } else if (directive == "index") {
        handleDirective(directive, builder,
            [](ConfigBuilder& b, const std::string& value) { b.setIndex(value); });
    } else if (directive == "client_max_body_size") {
        uint64_t size = readNumber("Expected body size");
        builder.setClientMaxBodySize(size);
        expectSemicolon();
    } else if (directive == "error_page") {
        uint64_t code = readNumber("Expected error code");
        if (code < 400 || code > 599) {
            throw ParseError("Invalid error code", valueToken);
        }
        std::string page = readValue("Expected error page path");
        builder.addErrorPage(static_cast<uint16_t>(code), page);
        expectSemicolon();
    } else {
        throw ParseError("Unknown server directive: " + directive, current_token_);
    }
}