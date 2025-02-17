#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>

namespace ConfigUtils
{
    std::vector<std::string> tokenize(const std::string& line);
    bool isCommentOrEmpty(const std::string& line);
    std::string extractBlockType(const std::string& line);
    bool isEndOfBlock(const std::string& line);
    std::string trimWhitespace(const std::string &line);
}

#endif
