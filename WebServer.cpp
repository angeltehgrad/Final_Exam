/*
 * WebServer.cpp
 *
 *  Created on: 24 февр. 2016 г.
 *      Author: user
 */

#include "WebServer.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

using std::string;

#define MAX_EVENTS 12

int set_nonblock(int fd)
{
int flags;
#if defined (O_NONBLOCK)
if(-1 == (flags = fcntl(fd, F_GETFL, 0)))
	flags = 0;
return fcntl(fd, F_SETFL, flags|O_NONBLOCK);
#else
	flags = 1;
	return ioctl(fd, FIOBIO, &flags);
#endif

}

struct ThreadArguments
{
	int sock;
	WebServer *server;
};


std::string CreateRespose(WebServer* srv, string msg)
{
	std::string res;
	string pathFile(srv->GetRoot());
	string filename;

	int pos = msg.find("GET");
	if(pos >= 0)
	{
		int begFile = msg.find("/", pos);
		if(begFile >= 0)
		{
			int endFile = msg.find("?", begFile);
			if(endFile < 0)
				{
				endFile = msg.find("HTTP");
				--endFile;
				}
			filename = msg.substr(begFile, endFile-begFile);
			pathFile += filename;
		}
		FILE *fp;
		fp = fopen(pathFile.c_str(), "r");
		if(fp == NULL)
		{
			std::string body = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
								"<html><head>"
								"<title>404 Not Found</title>"
								"</head><body>"
								"<h1>Not Found</h1>";

			char ansBuf[128] = {0};
			sprintf(ansBuf, "<p>The requested URL %s was not found on this server.</p>", filename.c_str());
			body += ansBuf;
			body += "</body></html>";
			res  = "HTTP/1.0 404 NOT FOUND\r\n"
					  "Content-length: ";
			char bufInt[12]={0};
			sprintf(bufInt, "%d", body.size());
			res += bufInt;
			res += "Content-Type: text/html\r\n" "\r\n";
			res += body;
			return res;
		}

		res = "HTTP/1.0 200 OK\r\n"
	     	     "Content-length: ";

		fseek(fp, 0, SEEK_END);
		unsigned int size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char *content = new char[size+1];
		if (size != fread(content, sizeof(char), size, fp)) {
			fclose(fp);
			delete[] content;
			return "ERROR";
		}
		content[size] = '\0';
		fclose(fp);
		char bufInt[12]={0};
		sprintf(bufInt, "%d", size);
		res += bufInt;
		res +="\r\n" "Content-Type: text/html\r\n" "\r\n";
		res += content;
		delete[] content;
		return res;
	}
	return res;
}

void *thread_func(void *arg)
{
    char Buffer[1024]={0};
    ThreadArguments *thArg = (ThreadArguments*)arg;
    WebServer *srv = thArg->server;
    int sock = thArg->sock;
    int RecvResult = recv(sock, Buffer, 1024, MSG_NOSIGNAL);
    if((RecvResult == 0)&&(errno != EAGAIN))
    {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
    else if(RecvResult > 0)
    {
    	string answer = CreateRespose(srv, Buffer);
    	send(sock, answer.c_str(), answer.size(), MSG_NOSIGNAL);
    	shutdown(sock, SHUT_RDWR);
    	close(sock);
    }
 return NULL;
}

WebServer::WebServer(int _port, char *_ipaddr, char * directory){

	port = _port;
	MasterSocket = -1;
	Epoll = -1;
	int len = strlen(_ipaddr);
	ipaddr = new char[len+1];
	strcpy(ipaddr, _ipaddr);
	ipaddr[len] = '\0';

	len = strlen(directory);
	if(directory[len-1] == '/'){
		--len;
	}
	rootDir = new char[len+1];
	strncpy(rootDir,directory, len);
	rootDir[len] = '\0';
}

WebServer::~WebServer() {
	delete[] ipaddr;
	delete[] rootDir;
}


void WebServer::Init()
{
	MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(port);
	SockAddr.sin_addr.s_addr = inet_addr(ipaddr);
	bind(MasterSocket, (struct sockaddr *) (&SockAddr), sizeof(SockAddr));
	set_nonblock(MasterSocket);
	listen(MasterSocket, SOMAXCONN);
	Epoll = epoll_create1(0);
    struct epoll_event Event;
    Event.data.fd = MasterSocket;
    Event.events = EPOLLIN;
    epoll_ctl(Epoll, EPOLL_CTL_ADD, MasterSocket, &Event);
}

void WebServer::Start()
{
    while(true)
    {
        struct epoll_event Events[MAX_EVENTS];
        int N = epoll_wait(Epoll, Events, MAX_EVENTS, -1);
        for(int i = 0; i < N; i++){
            if(Events[i].data.fd == MasterSocket)
            {
                int SlaveSocket = accept(MasterSocket, 0, 0);
                set_nonblock(SlaveSocket);
                struct epoll_event Event;
                Event.data.fd = SlaveSocket;
                Event.events = EPOLLIN;
                epoll_ctl(Epoll, EPOLL_CTL_ADD, SlaveSocket, &Event);
            }
            else
            {
            	pthread_t threadid;
            	pthread_attr_t attr;
            	pthread_attr_init(&attr);
            	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            	ThreadArguments arg;
            	arg.sock = Events[i].data.fd;
            	arg.server = this;
            	epoll_ctl(Epoll, EPOLL_CTL_DEL, Events[i].data.fd, &Events[i]);
            	pthread_create(&threadid, &attr, thread_func, &arg);
            }
        }
    }

}
