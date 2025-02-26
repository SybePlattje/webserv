#include "ConfigLexer.hpp"
#include <cctype>

void ConfigLexer::advance() {
    if (input_.get(current_char_)) {
        column_++;
        if (current_char_ == '\n') {
            line_++;
            column_ = 0;
        }
    } else {
        current_char_ = '\0';  // End of file
    }
}

char ConfigLexer::peek() {
    return input_.peek();
}

void ConfigLexer::skipWhitespace() {
    while (std::isspace(current_char_)) {
        advance();
    }
}

void ConfigLexer::skipComment() {
    while (current_char_ != '\n' && current_char_ != '\0') {
        advance();
    }
    if (current_char_ == '\n') {
        advance();
    }
}

Token ConfigLexer::readIdentifier() {
    std::string identifier;
    const size_t start_column = column_;

    // Allow alphanumeric characters, underscore, hyphen, forward slash, and dot
    while (std::isalnum(current_char_) || current_char_ == '_' || 
           current_char_ == '-' || current_char_ == '/' || current_char_ == '.') {
        identifier += current_char_;
        advance();
    }

    return Token(TokenType::IDENTIFIER, identifier, line_, start_column);
}

Token ConfigLexer::readNumber() {
    std::string number;
    const size_t start_column = column_;

    while (std::isdigit(current_char_)) {
        number += current_char_;
        advance();
    }

    return Token(TokenType::NUMBER, number, line_, start_column);
}

Token ConfigLexer::readString() {
    std::string str;
    const size_t start_column = column_;
    advance(); // Skip opening quote

    while (current_char_ != '"' && current_char_ != '\0') {
        if (current_char_ == '\n') {
            return Token(TokenType::INVALID, "Unterminated string literal", line_, start_column);
        }
        str += current_char_;
        advance();
    }

    if (current_char_ == '"') {
        advance(); // Skip closing quote
        return Token(TokenType::STRING, str, line_, start_column);
    }

    return Token(TokenType::INVALID, "Unterminated string literal", line_, start_column);
}

Token ConfigLexer::makeToken(TokenType type, const std::string& value) {
    return Token(type, value, line_, column_ - value.length());
}

Token ConfigLexer::makeError(const std::string& message) {
    error_ = message;
    return Token(TokenType::INVALID, message, line_, column_);
}

Token ConfigLexer::nextToken() {
    skipWhitespace();

    if (current_char_ == '\0') {
        return makeToken(TokenType::END_OF_FILE, "");
    }

    // Handle comments
    if (current_char_ == '#') {
        skipComment();
        return nextToken();
    }

    // Handle special characters
    switch (current_char_) {
        case '{':
            advance();
            return makeToken(TokenType::LBRACE, "{");
        case '}':
            advance();
            return makeToken(TokenType::RBRACE, "}");
        case ';':
            advance();
            return makeToken(TokenType::SEMICOLON, ";");
        case '"':
            return readString();
    }

    // Handle numbers
    if (std::isdigit(current_char_)) {
        return readNumber();
    }

    // Handle identifiers (allow starting with letter, underscore, or forward slash)
    if (std::isalpha(current_char_) || current_char_ == '_' || current_char_ == '/') {
        return readIdentifier();
    }

    // Invalid character
    std::string invalid(1, current_char_);
    advance();
    return makeError("Invalid character: " + invalid);
}