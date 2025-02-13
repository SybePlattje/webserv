#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <iostream>
#include <sstream>
#include <vector>

class Location
{
	public:
		Location(const std::string& path);
		~Location();
	
		void parse(std::istream& stream);
		void printLocation() const; // For debugging

	private:
		std::string					path_;
		std::string					root_;
		std::string					index_;
		std::vector<std::string>	allowed_methods_;
		bool						autoindex_;

		std::vector<std::string> tokenize(const std::string& line);
};

#endif