/*****************************************************
                PROJECT  : IO Catcher
                LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <climits>
#include <cassert>
//unix
#include <unistd.h>
//socket
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
//internal
#include "TcpServer.hpp"
#include "../common/Debug.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
#define MAX_TCP_MSG_LEN 4096
#define trace(args...) printf(args)

/****************************************************/
TcpServer::TcpServer(int port, int portRange, const std::string & hgConnectInfo)
{
	int res = createSocketV4(port, port + portRange, NULL);
	assumeArg(res >= 0, "Failed to open socket : %1").arg(res).end();
	this->listenFd = res;
	this->nr_clients = 0;
	this->hgConnectInfo = hgConnectInfo;
}

/****************************************************/
TcpServer::~TcpServer(void)
{
	int res = close(this->listenFd);
	assumeArg(res >= 0, "Failed to close socket : %1").arg(res).end();
}

/****************************************************/
int TcpServer::setupLibeventListener(void)
{
	this->ebase = event_base_new();
	assume(ebase != NULL, "Failed to allocate a new event base\n");

	struct event *evt = event_new(ebase, this->listenFd, EV_READ | EV_PERSIST, acceptOneClientCallback, this);
	assume(evt != NULL, "Failed to allocate a new event\n");

	event_add(evt, NULL);
	return 0;

}

/****************************************************/
void TcpServer::sendClientId(TcpClientInfo *client)
{
	uint64_t id;
	uint64_t key;
	//client_id_new(idtable, &id, &key, (struct client_data*) client);
	this->onConnect(&id, &key, client);
	client->id = id;
	int rc;
	rc = write(client->fd, &id, sizeof(id));
	assume(rc == sizeof(id), "Invalid write() size !");
	rc = write(client->fd, &key, sizeof(key));
	assume(rc == sizeof(key), "Invalid write() size !");
}

/****************************************************/
void TcpServer::sendHgConnectInfo(TcpClientInfo *client)
{
	const char *info = this->hgConnectInfo.c_str();
	size_t rc = write(client->fd, info, strlen(info));
	assume(rc == strlen(info), "Invalid write() size !");
}

/****************************************************/
void TcpServer::acceptOneClientCallback(evutil_socket_t fd, short unused_events, void * tcpServer)
{
	TcpServer * server = static_cast<TcpServer *>(tcpServer);

	trace("accept on fd %d\n", fd);
	int newfd = accept(fd, NULL, 0);
	if (newfd < 0) {
		fprintf(stderr, "accept failed: %s\n", strerror(errno));
		return;
	}
	trace("newfd %d\n",  newfd);

	TcpClientInfo * client = new TcpClientInfo;
	client->id = ULLONG_MAX;
	struct event *evt = event_new(server->ebase, newfd, EV_READ | EV_PERSIST, recvClientMessageCallback, client);
	if (!evt) {
		fprintf(stderr, "Failed to allocate a new event\n");
		close(newfd);
		delete client;
		return;
	}
	client->event = evt;
	client->fd = newfd;
	client->server = server;

	event_add(evt, NULL);

	printf("Client %p arrived\n", client);
	server->sendClientId(client);
	server->sendHgConnectInfo(client);
}

void TcpServer::recvClientMessageCallback(evutil_socket_t fd, short unused_events, void *clnt)
{
	/** Buffer for reception of messages */
	static char msg[MAX_TCP_MSG_LEN];
	TcpClientInfo * client = static_cast<TcpClientInfo *>(clnt);
	ssize_t n;

retry:
	int max_msg_len = MAX_TCP_MSG_LEN;
	n = read(fd, msg, max_msg_len);
	if (n == 0) {
		client->server->closeClient(client, fd);
	} else if (n > 0) {
		trace("new message size %ld from client fd %d\n", n, fd);
		handleClientMessage(client, msg, n);
	} else if (n < 0) {
		if (errno == EINTR) {
			goto retry;
		} else {
			fprintf(stderr, "Failed to receive a message : %s\n", strerror(errno));
		}
	}
}

/****************************************************/
void TcpServer::stop(void)
{
	event_base_loopbreak(this->ebase);
}

/****************************************************/
void TcpServer::handleClientMessage(TcpClientInfo *client, const char *msg, int len)
{
	// do some stuff here
	printf("Client %p sent msg of len %d\n", client, len);
	if (strncmp(msg,"stop", 4) == 0)
		client->server->stop();
}

/****************************************************/
void TcpServer::closeClient(TcpClientInfo *client, int fd)
{
	trace("client fd %d exits\n", fd);
	event_del(client->event);
	event_free(client->event);
	close(fd);
	this->onDisconnect(client);
	//handle_client_disconnect(client);
	delete client;
}

/****************************************************/
void TcpServer::loop(std::function<void(uint64_t*, uint64_t*, TcpClientInfo*)> onConnect,std::function<void(TcpClientInfo*)> onDisconnect)
{
	this->onConnect = onConnect;
	this->onDisconnect = onDisconnect;

	setupLibeventListener();
	trace("Dispatch starts\n");
	event_base_dispatch(ebase);
	trace("Dispatch stopped\n");
	// Hmm. Proper libevent cleanup should take place here.
}

/****************************************************/
int TcpServer::createSocketV4(int firstPort, int maxPort, int *bound_port)
{
	int			sock = -1;
	int			one = 1;
	int			centvingt = 120;
	int			neuf = 9;
	int			rc = -1;
	struct	sockaddr_in	sinaddr;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1) {
		fprintf(stderr, "Error creating an IPv4 socket: %s (%d)\n", strerror(errno), errno);
		return -1;
	}

	if ((-1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) ||
	    (-1 == setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one))) ||
	    (-1 == setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &centvingt, sizeof(centvingt))) ||
	    (-1 == setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &centvingt, sizeof(centvingt))) ||
	    (-1 == setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &neuf, sizeof(neuf)))) {
		fprintf(stderr, "Error setting a IPv4 socket's options: %s (%d)\n",
		          strerror(errno), errno);
		goto err;
	}

	// socket_setoptions(sock); tries to max out the msg size -- probably not needed
	memset(&sinaddr, 0, sizeof(sinaddr));
	sinaddr.sin_family = AF_INET;

	/* All the interfaces on the machine are used */
	sinaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	in_port_t port;
	for (port = firstPort; port <= maxPort; port++) {
		sinaddr.sin_port = htons(port);
		rc = bind(sock, (struct sockaddr *) &sinaddr, sizeof(sinaddr));
		if (!rc || errno != EADDRINUSE)
			break;
	}
	if (rc) {
		fprintf(stderr, "Cannot bind an IPv4 socket: %s (%d)\n", strerror(errno), errno);
		goto err;
	}

	if (-1 == listen(sock, 20)) {
		fprintf(stderr, "Failed to listen on an IPv4 socket: %s (%d)\n",
		          strerror(errno), errno);
		goto err;
	}

	trace("Open port %d\n", port);
	if (bound_port)
		*bound_port = port;
	return sock;

err:
	if (close(sock) < 0)
		fprintf(stderr, "Failed to close an IPv4 socket: %s (%d)\n", strerror(errno), errno);
	return -1;
}
