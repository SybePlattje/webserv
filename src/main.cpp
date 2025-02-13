#include "Config.hpp"

int main(int argc, char *argv[])
{
		(void) argc;
		Config  config(argv[1]);
		config.printConfig();
}
