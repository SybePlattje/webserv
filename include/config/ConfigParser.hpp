#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ConfigLexer.hpp"
#include "ConfigBuilder.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>

/**
 * @brief Parser for server configuration files
 *
 * Parses NGINX-style configuration files into Config objects.
 * Handles server and location blocks, directives, and their values.
 * Provides detailed error reporting with line and column information.
 */
class ConfigParser {
public:
    /**
     * @brief Exception class for parsing errors
     *
     * Provides detailed error information including the position
     * where the error occurred in the source file.
     */
    class ParseError : public std::runtime_error {
    public:
        /**
         * @brief Creates a parse error with position information
         * @param msg Error message
         * @param token Token where the error occurred
         * @param useEndPosition Whether to use token end (true) or start (false) position
         */
        ParseError(const std::string& msg, const Token& token, bool useEndPosition = false)
            : std::runtime_error(formatError(msg, useEndPosition ? token.end : token.start))
            , position_(useEndPosition ? token.end : token.start) {}

        /**
         * @return Position where the error occurred
         */
        const Position& getPosition() const { return position_; }

    private:
        static std::string formatError(const std::string& msg, const Position& pos) {
            return msg + " at " + pos.toString();
        }

        Position position_;  ///< Error location in source
    };

    /**
     * @brief Parses configuration from an input stream
     * @param input Stream containing configuration data
     * @return Vector of unique pointers to parsed Config objects, one for each server block
     * @throws ParseError on syntax or semantic errors
     */
    static std::vector<std::unique_ptr<Config>> parse(std::istream& input);

private:
    /**
     * @brief Creates parser with associated lexer
     * @param lexer Lexer for tokenizing input
     */
    ConfigParser(ConfigLexer& lexer) : lexer_(lexer) {}

    // Type definitions for directive handling
    using DirectiveHandler = std::function<void(ConfigBuilder&, const std::string&)>;
    using ValueValidator = std::function<void(const Token&)>;

    // Main parsing methods
    std::vector<std::unique_ptr<Config>> parseConfigs();
    std::unique_ptr<Config> parseServerBlock();
    void parseServerBlockContent(ConfigBuilder& builder);

    /**
     * @brief Parses a location block
     * @param builder Configuration builder to store settings
     * @throws ParseError on invalid location block syntax
     */
    void parseLocationBlock(ConfigBuilder& builder);

    /**
     * @brief Parses a configuration directive
     * @param builder Configuration builder to store settings
     * @param in_location True if parsing inside a location block
     * @throws ParseError on invalid directive syntax
     */
    void parseDirective(ConfigBuilder& builder, bool in_location);

    // Location directive handlers
    void parseLocationDirective(ConfigBuilder& builder, const std::string& directive);
    void parseLocationRoot(ConfigBuilder& builder);
    void parseLocationIndex(ConfigBuilder& builder);
    void parseLocationMethods(ConfigBuilder& builder);
    void parseLocationAutoindex(ConfigBuilder& builder);
    void parseLocationReturn(ConfigBuilder& builder);
    void parseLocationCGIPath(ConfigBuilder& builder);
    void parseLocationCGIExt(ConfigBuilder& builder);

    // Server directive handlers
    void parseServerDirective(ConfigBuilder& builder, const std::string& directive);
    void parseServerListen(ConfigBuilder& builder);
    void parseServerName(ConfigBuilder& builder);
    void parseServerRoot(ConfigBuilder& builder);
    void parseServerIndex(ConfigBuilder& builder);
    void parseServerBodySize(ConfigBuilder& builder);
    void parseServerErrorPage(ConfigBuilder& builder);

    /**
     * @brief Generic directive handler with validation
     * @param directive Name of the directive being parsed
     * @param builder Configuration builder to store settings
     * @param handler Function to process the directive value
     * @param validator Optional function to validate the value
     * @throws ParseError on invalid directive syntax or value
     */
    void handleDirective(const std::string& directive, 
                        ConfigBuilder& builder,
                        const DirectiveHandler& handler,
                        const ValueValidator& validator = nullptr);
    
    // Value reading utilities
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
    ConfigLexer& lexer_;  ///< Lexer providing tokens
    Token current_token_{TokenType::INVALID, "", Position(), Position()};
    Token valueToken{TokenType::INVALID, "", Position(), Position()};  ///< Last value token
};

#endif