//============================================================================
// Name        : Web_server.cpp
// Author      : Alexey
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "WebServer.h"
#include "tools.h"
#include <cstdlib>
#include <syslog.h>
#include <sys/stat.h>

#define DAEMON_NAME "WebServerdaemon"

int main(int argc, char **argv) {
	char *port;
	char *ip;
	char *dir;
	int c;
	while ((c = getopt (argc, argv, "h:p:d:")) != -1)
	{
		switch (c) {
			case 'h':
				ip = optarg;
				break;
			case 'p':
				port = optarg;
				break;
			case 'd':
				dir = optarg;
				break;
			default:
				abort();
		}
	}

	pid_t pid, sid;
	pid = fork();

    if (pid < 0) { exit(EXIT_FAILURE); }

    //We got a good pid, Close the Parent Process
    if (pid > 0) { exit(EXIT_SUCCESS); }

    //Change File Mask
    umask(0);
    sid = setsid();
    if (sid < 0) { exit(EXIT_FAILURE); }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

	WebServer testServer(atoi(port), ip, dir);
	testServer.Init();
	testServer.Start();
	return 0;
}
