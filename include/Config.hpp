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
		uint						listen_;
		std::string					server_name_;
		std::string 				host_;
		std::string					root_;
		std::string 				index_;
		std::map<uint, std::string>	error_pages_;
		uint						client_max_body_size_;
		std::vector<Location>		locations_;
		
		std::vector<std::string> tokenize(const std::string& line);
		void parseServerBlock(std::istream& stream);
		void parseLocationBlock(std::istream& stream, const std::string& path);
		bool isCommentOrEmpty(const std::string& line);
		std::string extractBlockType(const std::string& line);
};

#endif