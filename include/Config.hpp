#ifndef	CONFIG_HPP
# define CONFIG_HPP

# include <iostream>
# include <vector>
# include <fstream>
# include <sstream>
# include <string>
# include <stdexcept>
# include <map>
# include "Location.hpp"

# define DEFAULT_CONFIG "./webserv.conf"

class Config
{
	public:
		Config(const char *argv);
		~Config();
	
		void printConfig() const; // For debugging purposes;
	private:
		std::string					server_name_;
		std::string 				host_;
		uint						listen_;
		std::string					root_;
		std::string 				index_;
		std::map<uint, std::string>	error_pages_;
		uint						client_max_body_size_;
		std::vector<Location>		locations_;
		
		void setConfigData(const std::string &fileContents);
		void parseLine(const std::string &line);
};

#endif