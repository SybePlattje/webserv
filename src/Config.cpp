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
	std::istringstream iss(line);
	std::string key, value;
	iss >> key >> value;

	if (key == "server_name")
		server_name_ = value;
	else if (key == "host")
		host_ = value;
	else if (key == "listen")
		listen_ = std::stoi(value);
	else if (key == "root")
		root_ = value;
	else if (key == "index")
		index_ = value;
	else if (key == "error_page_404")
		error_page_404_ = value;
	else if (key == "client_max_body_size")
		client_max_body_size_ = std::stoi(value);
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
			  << "Error Page 404: " << error_page_404_ << "\n"
			  << "Client Max Body Size: " << client_max_body_size_ << "\n"
			  << "Number of Locations: " << locations_.size() << std::endl;
}