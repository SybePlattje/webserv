#include "Config.hpp"

/**
 * @brief Constructs a Config object.
 * 
 * This constructor initializes a Config object by reading the configuration
 * file specified by the input argument. If the input argument is null, it 
 * defaults to using a predefined configuration file path (DEFAULT_CONFIG).
 * 
 * @param argv The path to the configuration file. If null, DEFAULT_CONFIG is used.
 * 
 * @throws std::runtime_error if the configuration file cannot be opened.
 */
Config::Config(const char *argv)
: listen_(9999), server_name_("localhost"), root_("/"), client_max_body_size_(0)
{
	std::string			line, block;
	const std::string 	path_to_config = argv ? argv : DEFAULT_CONFIG;
	std::ifstream		configFile(path_to_config);

	if (!configFile.is_open())
		throw std::runtime_error("Config: Unable to open configuration file: " + path_to_config);
	while (getline(configFile, line))
	{
		if (ConfigUtils::isCommentOrEmpty(line))
			continue ;
		if (line.find("{") != std::string::npos)
		{
			std::string blockType = ConfigUtils::extractBlockType(line);
			if (blockType == "server")
			{
				parseServerBlock(configFile);
			}
			else
				throw std::runtime_error("Config: Unsupported block type: " + blockType);
		}
	}
	configFile.close();
}

Config::~Config() {}

Config::Config(const Config& other)
{
	*this = other;
}

Config& Config::operator=(const Config& other)
{
	if (this != &other)
	{
		server_name_ = other.server_name_;
		listen_ = other.listen_;
		root_ = other.root_;
		index_ = other.index_;
		error_pages_ = other.error_pages_;
		client_max_body_size_ = other.client_max_body_size_;
		locations_ = other.locations_;
	}
	return *this;
}

void Config::printConfig() const
{
	std::cout << "Server Name: " << server_name_ << "\n"
			  << "Listen Port: " << listen_ << "\n"
			  << "Root: " << root_ << "\n"
			  << "Index: " << index_ << "\n"
			  << "Error Pages: \n";
	for (const auto &entry : error_pages_)
	{
		std::cout << "  Code " << entry.first << ": " << entry.second << "\n";
	}
	std::cout << "Client Max Body Size: " << client_max_body_size_ << "\n"
			  << "Number of Locations: " << locations_.size() << std::endl;
}

void Config::parseServerBlock(std::istream& stream)
{
	std::string line;

	while (getline(stream, line))
	{
		if (ConfigUtils::isEndOfBlock(line))
			break;

		std::vector<std::string> tokens = ConfigUtils::tokenize(line);
		if (tokens.empty())
			continue ;

		const std::string& key = tokens[0];
		if (key == "listen")
			listen_ = std::stoi(tokens[1]);
		else if (key == "server_name")
			server_name_ = tokens[1];
		else if (key == "root")
			root_ = tokens[1];
		else if (key == "index")
			index_ = tokens[1];
		else if (key == "error_page")
			error_pages_[std::stoi(tokens[1])] = tokens[2];
		else if (key == "location")
			parseLocationBlock(stream, tokens[1]);
		else if (key == "client_max_body_size")
			client_max_body_size_ = std::stoi(tokens[1]);
		else
			throw std::runtime_error("Unknown directive: " + key);
	}
}

void Config::parseLocationBlock(std::istream& stream, const std::string& path)
{
	Location	loc(path);

	loc.parse(stream);
	locations_.push_back(loc);
}

const uint& Config::getListen()
{
	return listen_;
}

const std::string& Config::getServerName()
{
	return server_name_;
}

const std::string& Config::getRoot()
{
	return root_;
}

const std::string& Config::getIndex()
{
	return index_;
}

const std::map<uint, std::string>& Config::getErrorPages()
{
	return error_pages_;
}

const uint& Config::getClientMaxBodySize()
{
	return client_max_body_size_;
}

const std::vector<Location>& Config::getLocations()
{
	return locations_;
}
