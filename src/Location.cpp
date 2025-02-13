#include "Location.hpp"

Location::Location(const std::string& path) : path_(path), autoindex_(false) {}

Location::~Location() {}

void Location::parse(std::istream& stream)
{
	std::string	line;

	while (getline(stream, line))
	{
		if (line.find("}") != std::string::npos) break;  // End of location block

		std::vector<std::string> tokens = tokenize(line);
		if (tokens.empty())
			continue;

		const std::string& key = tokens[0];
		if (key == "root")
			root_ = tokens[1];
		else if (key == "index")
			index_ = tokens[1];
		else if (key == "allow_methods")
		{
			for (size_t i = 1; i < tokens.size(); ++i)
				allowed_methods_.push_back(tokens[i]);
		}
		else if (key == "autoindex")
			autoindex_ = (tokens[1] == "on");
	}
}

std::vector<std::string> Location::tokenize(const std::string& line)
{
	std::istringstream			iss(line);
	std::vector<std::string>	tokens;
	std::string					token;

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

void Location::printLocation() const
{
	std::cout << "Location Path: " << path_ << "\n"
			  << "Root: " << root_ << "\n"
			  << "Index: " << index_ << "\n"
			  << "Allowed Methods: ";
	for (const auto& method : allowed_methods_)
		std::cout << method << " ";
	std::cout << "\nAutoindex: " << (autoindex_ ? "on" : "off") << std::endl;
}
