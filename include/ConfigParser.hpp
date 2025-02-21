#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include "IConfiguration.hpp"
#include "ConfigurationBuilder.hpp"
#include "ConfigValidator.hpp"

class ConfigParser {
public:
    class ParserException : public std::runtime_error {
    public:
        ParserException(const std::string& msg, size_t line_number) 
            : std::runtime_error("Line " + std::to_string(line_number) + ": " + msg) {}
    };

    // Parse configuration from file
    static std::unique_ptr<IConfiguration> parseFile(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            throw ParserException("Failed to open config file: " + path, 0);
        }
        
        ConfigurationBuilder builder;
        parseConfiguration(file, builder);
        auto config = builder.build();
        ConfigValidator::validateConfiguration(*config);
        return config;
    }

private:
    // Prevent instantiation
    ConfigParser() = delete;
    
    static void parseConfiguration(std::istream& stream, ConfigurationBuilder& builder);
    static void parseServerBlock(std::istream& stream, ConfigurationBuilder& builder, size_t& line_number);
    static void parseLocationBlock(std::istream& stream, const std::string& path, 
                                 ConfigurationBuilder& builder, size_t line_number);
    
    static bool isBlockStart(const std::string& line, std::string& blockType, 
                           std::string& blockPath, size_t line_number);
    static bool isBlockEnd(const std::string& line);
    
    static std::vector<std::string> tokenizeLine(const std::string& line);
    static void handleDirective(const std::string& directive, 
                              const std::vector<std::string>& tokens,
                              ConfigurationBuilder& builder,
                              size_t line_number);
                              
    static void validateDirective(const std::string& directive, 
                                const std::vector<std::string>& tokens,
                                size_t expected_tokens,
                                size_t line_number);
};

#endif