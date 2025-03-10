#include "Config.hpp"
#include <cctype>

void ConfigLexer::advance() {
    if (input_.get(current_char_)) {
        if (current_char_ == '\n') {
            current_pos_.line++;
            current_pos_.column = 0;
        } else {
            current_pos_.column++;
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
    Position start = current_pos_;
    Position end;  // Track end position before advancing to next token

    // Allow alphanumeric characters, underscore, hyphen, forward slash, and dot
    while (std::isalnum(current_char_) || current_char_ == '_' ||
           current_char_ == '-' || current_char_ == '/' || current_char_ == '.') {
        identifier += current_char_;
        end = current_pos_;  // Save position before advancing
        advance();
    }

    if (end.line == 0) {  // If no end position was saved (empty identifier)
        end = start;
    }

    return Token(TokenType::IDENTIFIER, identifier, start, end);
}

Token ConfigLexer::readNumber() {
    std::string number;
    Position start = current_pos_;
    Position end;  // Track end position before advancing to next token

    while (std::isdigit(current_char_)) {
        number += current_char_;
        end = current_pos_;  // Save position before advancing
        advance();
    }

    if (end.line == 0) {  // If no end position was saved (empty number)
        end = start;
    }

    return Token(TokenType::NUMBER, number, start, end);
}

Token ConfigLexer::readString() {
    std::string str;
    Position start = current_pos_;
    advance(); // Skip opening quote

    Position end;  // Track end position before advancing to next token
    while (current_char_ != '"' && current_char_ != '\0') {
        if (current_char_ == '\n') {
            return Token(TokenType::INVALID, "Unterminated string literal", start, current_pos_);
        }
        str += current_char_;
        end = current_pos_;  // Save position before advancing
        advance();
    }

    if (current_char_ == '"') {
        end = current_pos_;  // Include the closing quote in the end position
        advance(); // Skip closing quote
        return Token(TokenType::STRING, str, start, end);
    }

    return Token(TokenType::INVALID, "Unterminated string literal", start, end.line == 0 ? start : end);
}

Token ConfigLexer::makeToken(TokenType type, const std::string& value, const Position& start) {
    return Token(type, value, start, current_pos_);
}

Token ConfigLexer::makeError(const std::string& message) {
    error_ = message;
    return Token(TokenType::INVALID, message, current_pos_, current_pos_);
}

Token ConfigLexer::nextToken() {
    skipWhitespace();

    if (current_char_ == '\0') {
        return makeToken(TokenType::END_OF_FILE, "", current_pos_);
    }

    // Handle comments
    if (current_char_ == '#') {
        skipComment();
        return nextToken();
    }

    // Handle special characters
    Position start = current_pos_;
    switch (current_char_) {
        case '{':
            advance();
            return makeToken(TokenType::LBRACE, "{", start);
        case '}':
            advance();
            return makeToken(TokenType::RBRACE, "}", start);
        case ';':
            advance();
            return makeToken(TokenType::SEMICOLON, ";", start);
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