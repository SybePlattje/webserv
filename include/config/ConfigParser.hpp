#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ConfigLexer.hpp"
#include "ConfigBuilder.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>

class ConfigParser {
public:
    // Parse exception with position information
    class ParseError : public std::runtime_error {
    public:
        ParseError(const std::string& msg, const Token& token, bool useEndPosition = false)
            : std::runtime_error(formatError(msg, useEndPosition ? token.end : token.start))
            , position_(useEndPosition ? token.end : token.start) {}

        const Position& getPosition() const { return position_; }

    private:
        static std::string formatError(const std::string& msg, const Position& pos) {
            return msg + " at " + pos.toString();
        }

        Position position_;  // Where the error occurred
    };

    // Parse from input stream
    static std::unique_ptr<Config> parse(std::istream& input);

private:
    ConfigParser(ConfigLexer& lexer) : lexer_(lexer) {}

    // Type definitions for directive handling
    using DirectiveHandler = std::function<void(ConfigBuilder&, const std::string&)>;
    using ValueValidator = std::function<void(const Token&)>;

    // Main parsing methods
    std::unique_ptr<Config> parseConfig();
    void parseServerBlock(ConfigBuilder& builder);
    void parseLocationBlock(ConfigBuilder& builder);
    void parseDirective(ConfigBuilder& builder, bool in_location);
    void parseReturn(ConfigBuilder& builder);

    // Helper methods for directive parsing
    void handleDirective(const std::string& directive, 
                        ConfigBuilder& builder,
                        const DirectiveHandler& handler,
                        const ValueValidator& validator = nullptr);
    
    std::string readValue(const std::string& error_msg);
    std::vector<std::string> readValueList(const std::string& error_msg);
    uint64_t readNumber(const std::string& error_msg);

    // Token handling
    Token getCurrentToken() const { return current_token_; }
    void advance();
    bool match(TokenType type);
    void expect(TokenType type, const std::string& error_msg);
    std::string expectIdentifier(const std::string& error_msg);
    std::vector<std::string> expectIdentifierList(const std::string& error_msg);
    uint64_t expectNumber(const std::string& error_msg);
    void expectSemicolon();

    // Member variables
    ConfigLexer& lexer_;
    Token current_token_{TokenType::INVALID, "", Position(), Position()};
    Token valueToken{TokenType::INVALID, "", Position(), Position()};  // Tracks last value token
};

#endif