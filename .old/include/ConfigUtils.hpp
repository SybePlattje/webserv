#ifndef CONFIG_UTILS_HPP
#define CONFIG_UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

namespace ConfigUtils {
    // Core text operations
    std::vector<std::string> splitIntoTokens(const std::string &line);
    std::string trimWhitespace(const std::string &line);
    
    // Syntax validation
    void checkForContentAfterSemicolon(std::istringstream& iss);
    void validateLineSyntax(const std::string& line, const std::vector<std::string>& tokens);
    bool needsSemicolon(const std::string& line);

    // Line type checking
    bool isCommentOrEmpty(const std::string &line);
    bool isEndOfBlock(const std::string &line);
    std::string extractBlockType(const std::string &line);
}

#endif
