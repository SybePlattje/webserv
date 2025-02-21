#include "ConfigUtils.hpp"
#include <iostream>

namespace ConfigUtils {

std::vector<std::string> splitIntoTokens(const std::string &line)
{
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;

    while (iss >> token)
    {
        // Stop at comments
        if (token[0] == '#')
            break;

        // Handle semicolon at end of token
        if (!token.empty() && token.back() == ';')
        {
            token.pop_back();
            if (!token.empty())
                tokens.push_back(token);
            
            checkForContentAfterSemicolon(iss);
            return tokens;
        }

        tokens.push_back(token);
    }

    return tokens;
}

void checkForContentAfterSemicolon(std::istringstream& iss)
{
    std::string token;
    while (iss >> token)
    {
        if (token[0] == '#')
            break;
        if (!token.empty())
            throw std::runtime_error("Config: Unexpected content after semicolon");
    }
}

void validateLineSyntax(const std::string& line, const std::vector<std::string>& tokens)
{
    if (tokens.empty())
        return;

    if (needsSemicolon(line))
    {
        // Get original line without comments
        std::string lineWithoutComments = line.substr(0, line.find('#'));
        
        // Check if line ends with semicolon (ignoring whitespace)
        size_t lastNonSpace = lineWithoutComments.find_last_not_of(" \t");
        if (lastNonSpace == std::string::npos || lineWithoutComments[lastNonSpace] != ';')
            throw std::runtime_error("Config: Missing semicolon at end of directive");
    }
}

bool needsSemicolon(const std::string& line)
{
    std::string trimmed = trimWhitespace(line);
    
    // First word of the line
    std::istringstream iss(trimmed);
    std::string firstWord;
    iss >> firstWord;

    // Don't require semicolon for:
    // 1. Empty lines or comments
    if (trimmed.empty() || trimmed[0] == '#')
        return false;
        
    // 2. Lines that are just a closing brace
    if (trimmed == "}")
        return false;
        
    // 3. Block declarations (server/location {...})
    if ((firstWord == "server" || firstWord == "location") && 
        trimmed.find("{") != std::string::npos)
        return false;

    return true;
}

bool isCommentOrEmpty(const std::string &line)
{
    std::string trimmed = trimWhitespace(line);
    return trimmed.empty() || trimmed[0] == '#';
}

std::string extractBlockType(const std::string &line)
{
    std::string trimmed = trimWhitespace(line);
    size_t firstSpace = trimmed.find_first_of(" \t");
    
    // If no space found or block type is empty, invalid format
    if (firstSpace == std::string::npos || firstSpace == 0)
        throw std::runtime_error("Config: Invalid block declaration");
        
    return trimmed.substr(0, firstSpace);
}

std::string trimWhitespace(const std::string &line)
{
    std::string trimmed = line;
    size_t start = trimmed.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    
    size_t end = trimmed.find_last_not_of(" \t");
    return trimmed.substr(start, end - start + 1);
}

bool isEndOfBlock(const std::string &line)
{
    std::string trimmed = trimWhitespace(line.substr(0, line.find("#")));
    return trimmed == "}";
}

} // namespace ConfigUtils
