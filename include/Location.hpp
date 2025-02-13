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
		Location(const Location& other);
		Location& operator=(const Location& other);
	
		void parse(std::istream& stream);
		const std::string& getPath() const;
		const std::string& getRoot() const;
		const std::string& getIndex() const;
		const std::vector<std::string>& getAllowedMethods() const;
		bool getAutoindex() const;
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