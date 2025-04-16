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

Position ConfigLexer::trackPosition() {
    Position pos = current_pos_;
    advance();
    return pos;
}

Token ConfigLexer::readWhile(std::function<bool(char)> predicate, TokenType type) {
    std::string content;
    Position start = current_pos_;
    Position end = start;

    while (predicate(current_char_)) {
        content += current_char_;
        end = current_pos_;
        advance();
    }

    return Token(type, content, start, end);
}

Token ConfigLexer::readIdentifier() {
    auto isValidIdentChar = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || 
               c == '_' || c == '-' || c == '/' || c == '.' ||
               c == '^' || c == '$' || c == '+' || c == '[' || c == ']' || 
               c == '(' || c == ')' || c == '\\' || c == '|';
    };
    return readWhile(isValidIdentChar, TokenType::IDENTIFIER);
}

Token ConfigLexer::readNumber() {
    auto isDigit = [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; };
    return readWhile(isDigit, TokenType::NUMBER);
}

Token ConfigLexer::readString() {
    std::string str;
    Position start = current_pos_;
    advance(); // Skip opening quote

    Position end = start;
    while (current_char_ != '"' && current_char_ != '\0') {
        if (current_char_ == '\n') {
            return Token(TokenType::INVALID, "Unterminated string literal", start, current_pos_);
        }
        str += current_char_;
        end = current_pos_;
        advance();
    }

    if (current_char_ == '"') {
        end = current_pos_;
        advance(); // Skip closing quote
        return Token(TokenType::STRING, str, start, end);
    }

    return Token(TokenType::INVALID, "Unterminated string literal", start, end);
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

    if (current_char_ == '#') {
        skipComment();
        return nextToken();
    }

    Position start = current_pos_;
    char c = current_char_;

    switch (c) {
        case '{': advance(); return makeToken(TokenType::LBRACE, "{", start);
        case '}': advance(); return makeToken(TokenType::RBRACE, "}", start);
        case ';': advance(); return makeToken(TokenType::SEMICOLON, ";", start);
        case '"': return readString();
        case '=': {
            advance();
            return makeToken(TokenType::MODIFIER, "=", start);
        }
        case '~': {
            advance();
            if (current_char_ == '*') {
                advance();
                return makeToken(TokenType::MODIFIER, "~*", start);
            }
            return makeToken(TokenType::MODIFIER, "~", start);
        }
        case '^': {
            advance();
            if (current_char_ == '~') {
                advance();
                return makeToken(TokenType::MODIFIER, "^~", start);
            }
            return readIdentifier();
        }
    }

    if (std::isdigit(static_cast<unsigned char>(current_char_))) {
        return readNumber();
    }

    if (std::isalpha(static_cast<unsigned char>(current_char_)) || 
        current_char_ == '_' || current_char_ == '/' ||
        current_char_ == '^' || current_char_ == '\\') {
        return readIdentifier();
    }

    std::string invalid(1, current_char_);
    advance();
    return makeError("Invalid character: " + invalid);
}