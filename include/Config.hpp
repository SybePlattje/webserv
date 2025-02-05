#ifndef	CONFIG_HPP
# define CONFIG_HPP

# include <iostream>
# include <vector>
# include <fstream>
# include <sstream>
# include <string>
# include "Location.hpp"

# define DEFAULT_CONFIG "./webserv.conf"

class Config
{
	public:
		Config::Config(const char *argv)
		~Config();
	private:
		std::string				server_name_;
		std::string 			host_;
		int						listen_;
		int						root_;
		std::string 			index_;
		std::string 			error_page_404_;
		int						client_max_body_size_;
		std::vector<Location>	locations_;
};

