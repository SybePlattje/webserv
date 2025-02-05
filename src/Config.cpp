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
		throw \
		std::runtime_error("config path " + path_to_config + " not found: No such file");
	}
	inputfile >> inputbuffer.rdbuf();
	inputfile.close();
	setConfigData(inputbuffer.str());
}

Config::~Config()
{
}


setConfigData