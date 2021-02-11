/*****************************************************
                PROJECT  : IO Catcher
                LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_TCP_SERVER_HPP
#define IOC_TCP_SERVER_HPP

/*****************************************************/
//std
#include <list>
#include <functional>
//libevent
#include <event2/event.h>

/*****************************************************/
namespace IOC
{

/*****************************************************/
class TcpServer;

/*****************************************************/
struct TcpClientInfo
{
	struct event *event; /**< libevent event for this client fd */
	int           fd;
	uint64_t      id;
	TcpServer *   server;
};

/*****************************************************/
class TcpServer
{
	public:
		TcpServer(int port, int portRange, const std::string & hgConnectInfo);
		~TcpServer(void);
		void loop(std::function<void(uint64_t*, uint64_t*, TcpClientInfo*)> onConnect,std::function<void(TcpClientInfo*)> onDisconnect);
		void stop(void);
	private:
		static int createSocketV4(int firstPort, int maxPort, int *bound_port);
		static void acceptOneClientCallback(evutil_socket_t fd, short unused_events, void * tcpServer);
		static void recvClientMessageCallback(evutil_socket_t fd, short unused_events, void *clnt);
		static void handleClientMessage(TcpClientInfo *client, const char *msg, int len);
	private:
		int setupLibeventListener(void);
		void sendClientId(TcpClientInfo *client);
		void sendHgConnectInfo(TcpClientInfo *client);
		void closeClient(TcpClientInfo *client, int fd);
	private:
		int listenFd;
		struct event_base *ebase;
		int nr_clients;
		std::string hgConnectInfo;
		std::list<TcpClientInfo> clients;
		std::function<void(uint64_t*, uint64_t*, TcpClientInfo*)> onConnect;
		std::function<void(TcpClientInfo*)> onDisconnect;
};

}


#endif //IOC_TCP_SERVER_HPP
