#ifndef	CONFIG_HPP
# define CONFIG_HPP

# include <iostream>
# include <vector>
# include <fstream>
# include <sstream>
# include <string>
# include <stdexcept>
# include <map>
# include "ConfigUtils.hpp"
# include "Location.hpp"

# define DEFAULT_CONFIG "./webserv.conf"

class Config
{
	public:
		Config(const char *argv);
		~Config();
		Config(const Config& other);
		Config& operator=(const Config& other);

		const uint& getListen();
		const std::string& getServerName();
		const std::string& getRoot();
		const std::string& getIndex();
		const std::map<uint, std::string>& getErrorPages();
		const uint& getClientMaxBodySize();
		const std::vector<Location>& getLocations();
		void printConfig() const; // For debugging purposes;

	private:
		uint						listen_;
		std::string					server_name_;
		std::string					root_;
		std::string					index_;
		std::map<uint, std::string>	error_pages_;
		uint						client_max_body_size_;
		std::vector<Location>		locations_;
		
		void parseServerBlock(std::istream& stream);
		void parseLocationBlock(std::istream& stream, const std::string& path);

};

#endif