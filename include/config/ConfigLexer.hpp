#ifndef CONFIG_LEXER_HPP
#define CONFIG_LEXER_HPP

#include <string>
#include <vector>
#include <istream>

// Token types
enum class TokenType {
    IDENTIFIER,     // Words like "server", "location", etc.
    NUMBER,         // Port numbers, sizes
    STRING,         // Quoted strings
    LBRACE,        // {
    RBRACE,        // }
    SEMICOLON,     // ;
    END_OF_FILE,   // End of input
    INVALID        // Invalid token
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;

    Token(TokenType t, const std::string& v, size_t l, size_t c)
        : type(t), value(v), line(l), column(c) {}
};

class ConfigLexer {
public:
    explicit ConfigLexer(std::istream& input) : input_(input), line_(1), column_(0) {}

    // Get the next token from input
    Token nextToken();

    // Error handling
    const std::string& getError() const { return error_; }
    bool hasError() const { return !error_.empty(); }

private:
    std::istream& input_;
    size_t line_;
    size_t column_;
    std::string error_;
    char current_char_ = ' ';

    // Helper methods
    void advance();
    char peek();
    void skipWhitespace();
    void skipComment();
    Token readIdentifier();
    Token readNumber();
    Token readString();
    Token makeToken(TokenType type, const std::string& value);
    Token makeError(const std::string& message);
};

#endif