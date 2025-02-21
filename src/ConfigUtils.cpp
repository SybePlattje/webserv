#include "ConfigUtils.hpp"

std::vector<std::string> ConfigUtils::tokenize(const std::string &line)
{
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;

    // First, get all tokens
    while (iss >> token)
    {
        // Stop at comments
        if (token[0] == '#')
            break;

        // If this token ends with semicolon, remove it and stop reading
        if (!token.empty() && token.back() == ';')
        {
            token.pop_back();
            if (!token.empty())
                tokens.push_back(token);
            
            // Check nothing comes after semicolon except comments
            while (iss >> token)
            {
                if (token[0] == '#')
                    break;
                if (!token.empty())
                    throw std::runtime_error("Config: Unexpected content after semicolon");
            }
            return tokens;
        }

        tokens.push_back(token);
    }

    // If we have tokens but no semicolon was found
    if (!tokens.empty())
    {
        std::string trimmed = trimWhitespace(line);
        bool isClosingBrace = trimmed == "}";
        bool hasOpenBrace = trimmed.find("{") != std::string::npos;

        // All lines except block declarations and closing braces must end with semicolon
        if (!hasOpenBrace && !isClosingBrace)
            throw std::runtime_error("Config: Missing semicolon at end of directive");
    }

    return tokens;
}

bool ConfigUtils::isCommentOrEmpty(const std::string &line)
{
    std::string trimmed = trimWhitespace(line);
    return trimmed.empty() || trimmed[0] == '#';
}

std::string ConfigUtils::extractBlockType(const std::string &line)
{
    std::string trimmed = trimWhitespace(line);
    size_t firstSpace = trimmed.find_first_of(" \t");
    if (firstSpace == std::string::npos)
        throw std::runtime_error("Config: Invalid block declaration");
        
    return trimmed.substr(0, firstSpace);
}

std::string ConfigUtils::trimWhitespace(const std::string &line)
{
    std::string trimmed = line;
    size_t start = trimmed.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    
    size_t end = trimmed.find_last_not_of(" \t");
    return trimmed.substr(start, end - start + 1);
}

bool ConfigUtils::isEndOfBlock(const std::string &line)
{
    std::string trimmed = trimWhitespace(line.substr(0, line.find("#")));
    return trimmed == "}";
}
