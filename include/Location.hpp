#include <iostream>

class Location
{
	public:
		enum class Methods
		{
			GET,
			POST,
			DELETE
		};
		Location(/* args */);
		~Location();
	private:
		bool 		methods_[3];
		bool		auto_index_;
		// int			nested_level_;
		std::string index_;
};

Location::Location(/* args */)
{
}

Location::~Location()
{
}
