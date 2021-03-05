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
/**
 * Constructor of the TCP client. It establish the connectin.
 * @param addr Address of the server.
 * @param port TCP port to connect to the server.
**/
TcpClient::TcpClient(const std::string & addr, const std::string & port)
{
	this->connFd = tcp_connect(addr.c_str(), port.c_str());
	assumeArg(this->connFd >= 0, "Fail to connect to server %1 !").arg(this->connFd).end();
}

/*****************************************************/
/**
 * Destructor to disconnect the client from the server.
**/
TcpClient::~TcpClient(void)
{
	if (this->connFd != -1)
		close(this->connFd);
}

/*****************************************************/
/**
 * Function used to etablish the conection to the server.
 * @param addr Address of the server.
 * @param aport TCP port to connect to the server.
 **/
int TcpClient::tcp_connect(const char *addr, const char *aport)
{
	int sockfd;
	struct sockaddr_in servaddr;
	unsigned short port;
	struct servent *srv;
	struct hostent *hp;

	//fprintf(stderr, "Connecting to %s:%s\n", addr, aport);

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
/**
 * After establishing the connection we make some read operation
 * on the connection to recive the auth information so we are
 * ready to exchange messages on the libfabric protocol.
 * @return Return a struct containing the auth informations recieved from the server.
**/
TcpConnInfo TcpClient::getConnectionInfos(void)
{
	// vars
	TcpConnInfo infos;

	// Get client ID and key from TCP server
	int rc;
	rc = read(connFd, &infos.clientId, sizeof(infos.clientId));
	assert(rc == sizeof(uint64_t));
	rc = read(connFd, &infos.key, sizeof(infos.key));
	assert(rc == sizeof(uint64_t));
	rc = read(connFd, &infos.keepConnection, sizeof(infos.keepConnection));
	assert(rc == sizeof(bool));
	//fprintf(stderr, "Got client ID %lu and key %lx and keep connection %d\n", infos.clientId, infos.key, (int)infos.keepConnection);

	//disconnect
	if (infos.keepConnection == false) {
		close(connFd);
		connFd = -1;
	}

	// ok
	return infos;
}
