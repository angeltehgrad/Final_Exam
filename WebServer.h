/*
 * WebServer.h
 *
 *  Created on: 24 февр. 2016 г.
 *      Author: user
 */

#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <unistd.h>
#include <string>
#include <cstdio>
#include <cstring>

class WebServer {
public:
	WebServer(int _port, char *_ipaddr, char * directory);
	void Init();
	void Start();
	char *GetRoot() {return rootDir;}
	~WebServer();


private:
	int port;
	char *ipaddr;
	char *rootDir;
	int MasterSocket;
	int Epoll;
};

#endif /* WEBSERVER_H_ */
