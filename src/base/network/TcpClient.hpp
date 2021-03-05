/*****************************************************
                PROJECT  : IO Catcher
                LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_TCP_CLIENT_HPP
#define IOC_TCP_CLIENT_HPP

/*****************************************************/
#include <string>

/*****************************************************/
namespace IOC
{

/*****************************************************/
/**
 * Describe a TCP connectoin info after the client establish
 * its first exchange with the server. This is used
 * to send auth informations in the libfabric messages.
**/
struct TcpConnInfo
{
	/** Contain the tcp client ID assigned by the server. **/
	uint64_t clientId;
	/** Authentication key assigned by the server. **/
	uint64_t key;
	/** 
	 * If we need to keep the TCP connection alive or if we can close it
	 * just after the first exchange. We keep it alive to track
	 * possible abort of the client as we cannot get a disconnection
	 * feedblock with the libfabric rxm protocol.
	**/
	bool keepConnection;
};

/*****************************************************/
/**
 * Tcp client object to handle a TCP connection to the server.
 * This is used to keep track of the client alive so the server
 * can be notified in case of a crash of the client.
 * This is required because libfabric rxm protocol do not notify
 * on client disconnection. This is required for range registration 
 * tracking to not block the server.
**/
class TcpClient
{
	public:
		TcpClient(const std::string & addr, const std::string & port);
		~TcpClient(void);
		TcpConnInfo getConnectionInfos(void);
	private:
		static int tcp_connect(const char *addr, const char *aport);
	private:
		/** File descriptor of the tcp connection. **/
		int connFd;
};

}

#endif //IOC_TCP_CLIENT_HPP
