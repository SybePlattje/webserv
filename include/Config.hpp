#include <iostream>
#include <vector>
#include <Location.hpp>

class Config
{
	public:
		Config(/* args */);
		~Config();
	private:
		std::string				server_name_;
		std::string 			host_;
		int						port_;
		int						root_;
		std::string 			index_;
		std::string 			error_page_404_;
		int						client_max_body_size_;
		std::vector<Location>	locations_;
};

Config::Config(/* args */)
{
}

Config::~Config()
{
}
