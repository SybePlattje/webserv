#include "Config.hpp"
#include "Server.hpp"

int main(int argc, char *argv[])
{
		(void) argc;
		Config  config(argv[1]);
		Server server(config);
		server.setupEpoll();
}
