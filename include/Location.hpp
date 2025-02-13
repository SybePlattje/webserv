#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <iostream>
#include <sstream>
#include <vector>

class Location
{
	public:
		enum class Methods
		{
			GET,
			POST,
			DELETE
		};
	
		Location();
		~Location();
	
		void parseLocation(std::istringstream &iss);
		bool isMethodAllowed(Methods method) const;
	
	private:
		bool methods_[3];
		bool auto_index_;
		std::string index_;
	
		Methods stringToMethod(const std::string &method);
};

#endif