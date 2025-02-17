#include "ConfigUtils.hpp"

std::vector<std::string> ConfigUtils::tokenize(const std::string &line)
{
	std::istringstream iss(line);
	std::vector<std::string> tokens;
	std::string token;

	while (iss >> token)
	{
		// Temporary solution (TODO: use trailing semicolon to check correct parsing)
		if (!token.empty() && token.back() == ';')
			token.pop_back();
		// \end temp

		if (token[0] == '#') // Ignore comments in the middle of lines
			break;
		tokens.push_back(token);
	}
	return tokens;
}

bool ConfigUtils::isCommentOrEmpty(const std::string &line)
{
	return line.empty() || line[0] == '#';
}

std::string ConfigUtils::extractBlockType(const std::string &line)
{
	std::istringstream blockStream(line.substr(0, line.find("{")));
	std::string blockType;

	blockStream >> blockType;
	return blockType;
}

std::string ConfigUtils::trimWhitespace(const std::string &line)
{
	std::string trimmed = line;
	trimmed.erase(0, trimmed.find_first_not_of(" \t"));
	trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
	return trimmed;
}

bool ConfigUtils::isEndOfBlock(const std::string &line)
{
	size_t closingBracePos = line.find("}");
	if (closingBracePos == std::string::npos)
		return false;
	std::string trimmedLine = trimWhitespace(line.substr(0, line.find("#")));
	trimmedLine = trimWhitespace(trimmedLine);

	if (trimmedLine.empty())
		return false;

	if (trimmedLine != "}")
		throw std::runtime_error("Config: Invalid content around closing brace \"" + line + "\"\n");
	return true;
}
