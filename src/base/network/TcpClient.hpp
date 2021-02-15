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
struct TcpConnInfo
{
	uint64_t clientId;
	uint64_t key;
	bool keepConnection;
};

/*****************************************************/
class TcpClient
{
	public:
		TcpClient(const std::string & addr, const std::string & port);
		~TcpClient(void);
		TcpConnInfo getConnectionInfos(void);
	private:
		static int tcp_connect(const char *addr, const char *aport);
	private:
		int connFd;
};

}

#endif //IOC_TCP_CLIENT_HPP
