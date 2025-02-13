#include "Location.hpp"

Location::Location() : auto_index_(false)
{
	methods_[0] = false; // GET
	methods_[1] = false; // POST
	methods_[2] = false; // DELETE
}

Location::~Location() {}

void Location::parseLocation(std::istringstream &iss)
{
	std::string token;
	while (iss >> token)
	{
		if (token == "methods")
		{
			std::string method;
			while (iss >> method && method != ";") // Assuming semicolon ends list
			{
				Methods m = stringToMethod(method);
				methods_[static_cast<int>(m)] = true;
			}
		}
		else if (token == "autoindex")
		{
			std::string value;
			iss >> value;
			auto_index_ = (value == "on");
		}
		else if (token == "index")
		{
			iss >> index_;
		}
		else
		{
			throw std::runtime_error("Unknown location directive: " + token);
		}
	}
}

bool Location::isMethodAllowed(Methods method) const
{
	return methods_[static_cast<int>(method)];
}

Location::Methods Location::stringToMethod(const std::string &method)
{
	if (method == "GET")
		return Methods::GET;
	if (method == "POST")
		return Methods::POST;
	if (method == "DELETE")
		return Methods::DELETE;
	throw std::runtime_error("Invalid HTTP method in location block: " + method);
}
