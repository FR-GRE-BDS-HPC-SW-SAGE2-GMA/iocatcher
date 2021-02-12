/*****************************************************
                PROJECT  : IO Catcher
                LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/*****************************************************/
//sys
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>             /* for sigaction */
#include <errno.h>
#include <netdb.h>
#include <cassert>
//local
#include "../common/Debug.hpp"
#include "TcpClient.hpp"

/*****************************************************/
using namespace IOC;

/*****************************************************/
#define MAXURL 256

/*****************************************************/
TcpClient::TcpClient(const std::string & addr, const std::string & port)
{
	this->connFd = tcp_connect(addr.c_str(), port.c_str());
	assumeArg(this->connFd >= 0, "Fail to connect to server %1 !").arg(this->connFd).end();
}

/*****************************************************/
TcpClient::~TcpClient(void)
{
	if (this->connFd != -1)
		close(this->connFd);
}

/*****************************************************/
int TcpClient::tcp_connect(const char *addr, const char *aport)
{
	int sockfd;
	struct sockaddr_in servaddr;
	unsigned short port;
	struct servent *srv;
	struct hostent *hp;

	fprintf(stderr, "Connecting to %s:%s\n", addr, aport);

	if (!addr || !aport)
		return -1;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;

	if (isdigit(addr[0]))
		servaddr.sin_addr.s_addr = inet_addr(addr);
	else {
		hp = gethostbyname(addr);
		if (hp == NULL) {
			herror("gethostbyname");
			exit(h_errno);
		}
		memcpy(&(servaddr.sin_addr.s_addr), hp->h_addr, hp->h_length);
	}

	if (isdigit(aport[0]))
		port = atoi(aport);
	else {
		srv = getservbyname(aport, "tcp");
		if (srv == NULL) {
			fprintf(stderr, "Can't resolve %s: %d(%s)\n",
				aport, errno, strerror(errno));
			exit(errno);
		}
		port = srv->s_port;
	}
	servaddr.sin_port = htons(port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		fprintf(stderr, "Can't get socket: %d(%s)\n",
				errno, strerror(errno));
		exit(errno);
	}

	if (connect(sockfd,
			(struct sockaddr *)&servaddr,
				sizeof(servaddr))) {
		fprintf(stderr, "connect failed: %d(%s)\n",
				errno, strerror(errno));
		close(sockfd);
		exit(errno);
	}

	return sockfd;
}

/*****************************************************/
TcpConnInfo TcpClient::getConnectionInfos(void)
{
	// vars
	TcpConnInfo infos;

	// Get client ID and key from TCP server
	int rc;
	rc = read(connFd, &infos.clientId, sizeof(uint64_t));
	assert(rc == sizeof(uint64_t));
	rc = read(connFd, &infos.key, sizeof(uint64_t));
	assert(rc == sizeof(uint64_t));
	fprintf(stderr, "Got client ID %lu and key %lx\n", infos.clientId, infos.key);

	// Get the mercury URL from the TCP server
	char url[MAXURL];
	ssize_t r = read(connFd, url, MAXURL-1);
	assert(r > 0);
	url[r] = '\0';
	fprintf(stderr, "Got connect url %s\n", url);
	infos.url = url;

	//disconnect
	if (strncmp("noauth", url, 7) == 0) {
		close(connFd);
		connFd = -1;
	}

	// ok
	return infos;
}
