#include "Config.hpp"
#include "Server.hpp"
#include <signal.h>

int main(int argc, char *argv[])
{
	::signal(SIGPIPE, SIG_IGN);
	(void) argc;
	Config  config(argv[1]);
	Server server(config);
	server.setupEpoll();
}
