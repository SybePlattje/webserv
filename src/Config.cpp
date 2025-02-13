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
{
	std::stringstream	inputbuffer;
	const std::string 	path_to_config = argv ? argv : DEFAULT_CONFIG;
	std::ifstream		inputfile(path_to_config);

	if (!inputfile.is_open())
	{
		throw std::runtime_error("Config file not found: " + path_to_config);
	}
	inputbuffer << inputfile.rdbuf();
	inputfile.close();
	setConfigData(inputbuffer.str());
}

Config::~Config() {}

void Config::setConfigData(const std::string &fileContents)
{
	std::istringstream stream(fileContents);
	std::string line;

	while (std::getline(stream, line))
	{
		if (line.empty() || line[0] == '#') // Ignore empty lines and comments
			continue;
		parseLine(line);
	}
}

void Config::parseLine(const std::string &line)
{
	std::istringstream	iss(line);
	std::string			key;
	iss >> key;

	if (key == "server_name")
		iss >> server_name_;
	else if (key == "host")
		iss >> host_;
	else if (key == "listen")
		iss >> listen_;
	else if (key == "root")
		iss >> root_;
	else if (key == "index")
		iss >> index_;
	else if (key == "error_page")
	{
		uint code;
		std::string filepath;
		iss >> code >> filepath;
		error_pages_[code] = filepath;
	}
	else if (key == "client_max_body_size")
		iss >> client_max_body_size_;
	else if (key == "location")
	{
		Location loc;
		loc.parseLocation(iss); // Parses remaining values in line
		locations_.push_back(loc);
	}
	else
	{
		throw std::runtime_error("Unknown directive in config file: " + key);
	}
}

void Config::printConfig() const
{
	std::cout << "Server Name: " << server_name_ << "\n"
			  << "Host: " << host_ << "\n"
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
