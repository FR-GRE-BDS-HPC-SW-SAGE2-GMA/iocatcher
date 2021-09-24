/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cstdlib>
#include <thread>
//gtest
#include <gtest/gtest.h>
//local
#include "../TcpClient.hpp"
#include "../TcpServer.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
// Test basic construction by starting a server and join by client.
TEST(TestTcpClientServer, constructor)
{
	//vars
	int cntConnect = 0;
	int cntDisconnect = 0;

	//server
	TcpServer server(5555,1,true);

	//server loop
	std::thread tcpThread([&server, &cntConnect, &cntDisconnect](){
		server.loop([&cntConnect](uint64_t* id, uint64_t* key, TcpClientInfo* client){
			cntConnect++;
			*id = 10;
			*key = 20;
		},[&cntDisconnect](TcpClientInfo* client){
			cntDisconnect++;
		});
	});

	//wait
	usleep(10000);

	//client
	TcpClient * client = new TcpClient("localhost", "5555");
	TcpConnInfo conn = client->getConnectionInfos();

	//check
	EXPECT_EQ(10, conn.clientId);
	EXPECT_EQ(20, conn.key);
	EXPECT_TRUE(conn.keepConnection);
	EXPECT_EQ(1, cntConnect);
	EXPECT_EQ(0, cntDisconnect);

	//disconnect
	delete client;
	usleep(10000);

	//check
	EXPECT_EQ(1, cntConnect);
	EXPECT_EQ(1, cntDisconnect);

	//stop server & join loop thread
	server.stop();

	//join
	tcpThread.join();
}
