#include "Configuration.hpp"

// Since all implementation is in the header file (inline methods)
// and the class is a friend of ConfigurationBuilder,
// we only need this file to provide the implementation point for virtual methods

// Define virtual destructor
Configuration::~Configuration() = default;