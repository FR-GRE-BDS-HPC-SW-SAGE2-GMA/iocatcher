/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

// This code is inspired from initial poc from Simon Derr.

#ifndef IOC_TCP_SERVER_HPP
#define IOC_TCP_SERVER_HPP

/*****************************************************/
//std
#include <list>
#include <functional>
//libevent
#include <event2/event.h>
#include <event2/thread.h>

/*****************************************************/
namespace IOC
{

/*****************************************************/
class TcpServer;

/*****************************************************/
/**
 * Struct used to keep track of client connection informations.
**/
struct TcpClientInfo
{
	/** libevent event for this client fd */
	struct event *event; 
	/** File descriptor of the connection to the client. **/
	int           fd;
	/** Client ID. **/
	uint64_t      id;
	/** Address of the TCP server. **/
	TcpServer *   server;
};

/*****************************************************/
/**
 * Implement the TCP server to keep track of client connection so we can
 * be notified when a client disconnect due to a crash. This is required
 * because libfabric rxm protocol do not provide this notification.
 * This is required for range registration tracking to not block the server.
**/
class TcpServer
{
	public:
		TcpServer(int port, int portRange, bool keepConnection);
		~TcpServer(void);
		void loop(std::function<void(uint64_t*, uint64_t*, TcpClientInfo*)> onConnect,std::function<void(TcpClientInfo*)> onDisconnect);
		void stop(void);
	private:
		static int createSocketV4(int firstPort, int maxPort, int *bound_port);
		static void acceptOneClientCallback(evutil_socket_t fd, short unused_events, void * tcpServer);
		static void recvClientMessageCallback(evutil_socket_t fd, short unused_events, void *clnt);
	private:
		int setupLibeventListener(void);
		void sendClientId(TcpClientInfo *client);
		void closeClient(TcpClientInfo *client, int fd);
	private:
		/** Listen socket **/
		int listenFd;
		/** Handler for libevent. **/
		struct event_base *ebase;
		/** Number of clients. **/
		int nr_clients;
		/** 
		 * To know if we need to keep the connection alive of if we disconnect
		 * just after exchanging the auth informaations.
		**/
		bool keepConnection;
		/** Track the alive clients. **/
		std::list<TcpClientInfo> clients;
		/** 
		 * Labmda function to be called when a client connection.
		 * It is used to fill the auth informations.
		**/
		std::function<void(uint64_t*, uint64_t*, TcpClientInfo*)> onConnect;
		/** Labmda function to be called when a client disconnect. **/
		std::function<void(TcpClientInfo*)> onDisconnect;
};

}

#endif //IOC_TCP_SERVER_HPP
