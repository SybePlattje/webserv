#ifndef CONFIG_LEXER_HPP
#define CONFIG_LEXER_HPP

#include <string>
#include <vector>
#include <istream>
#include <functional>

/**
 * @brief Position in source file
 */
struct Position {
    size_t line;
    size_t column;

    Position(size_t l = 1, size_t c = 0) : line(l), column(c) {}

    // Format position for error messages
    std::string toString() const {
        return "Line " + std::to_string(line) + ", Column " + std::to_string(column);
    }

    // Compare positions
    bool operator==(const Position& other) const {
        return line == other.line && column == other.column;
    }
};

/**
 * @brief Token types for configuration lexical analysis
 */
enum class TokenType {
    IDENTIFIER,     // Words like "server", "location", etc.
    NUMBER,         // Port numbers, sizes
    STRING,         // Quoted strings
    LBRACE,        // {
    RBRACE,        // }
    SEMICOLON,     // ;
    MODIFIER,      // Location modifiers (~, ~*, ^~, =)
    END_OF_FILE,   // End of input
    INVALID        // Invalid token
};

/**
 * @brief Token with precise position information
 */
struct Token {
    TokenType type;
    std::string value;
    Position start;    // Start of token
    Position end;      // End of token (position after last character)

    Token(TokenType t, const std::string& v, const Position& s, const Position& e)
        : type(t), value(v), start(s), end(e) {}

    // For single-character tokens or when end position isn't needed
    Token(TokenType t, const std::string& v, const Position& pos)
        : type(t), value(v), start(pos), end(pos) {}

    // Format position for error messages
    std::string getPositionString() const {
        return start.toString();
    }
};

/**
 * @brief Configuration lexer for tokenizing NGINX-style config files
 */
class ConfigLexer {
public:
    explicit ConfigLexer(std::istream& input)
        : input_(input)
        , current_pos_(1, 0)  // Start at line 1, column 0
        , current_char_(' ') {}

    // Get the next token from input
    Token nextToken();

    // Error handling
    const std::string& getError() const { return error_; }
    bool hasError() const { return !error_.empty(); }

private:
    std::istream& input_;
    Position current_pos_;     // Current position in source
    std::string error_;        // Last error message
    char current_char_;        // Current character

    // Basic character handling
    void advance();            // Move to next character
    char peek();              // Look at next character
    Position trackPosition(); // Track position before advancing
    
    // Skipping methods
    void skipWhitespace();    // Skip whitespace characters
    void skipComment();       // Skip from # to end of line
    
    // Token reading
    Token readWhile(std::function<bool(char)> predicate, TokenType type);
    Token readIdentifier();   // Read identifiers (including regex patterns)
    Token readNumber();       // Read numeric tokens
    Token readString();       // Read quoted strings
    
    // Token creation
    Token makeToken(TokenType type, const std::string& value, const Position& start);
    Token makeError(const std::string& message);
};

#endif