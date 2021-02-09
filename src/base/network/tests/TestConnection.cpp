/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include <thread>
#include <cstring>
#include "../LibfabricConnection.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
TEST(TestConnection, connect)
{
	bool gotConnection = false;
	std::thread server([&gotConnection]{
		LibfabricDomain domain("127.0.0.1", "8444", true);
		domain.setMsgBuffeSize(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_WRITE));
		LibfabricConnection connection(&domain, false);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection = true;
		});
		while (!gotConnection)
			connection.poll(false);
	});
	std::thread client([]{
		LibfabricDomain domain("127.0.0.1", "8444", false);
		domain.setMsgBuffeSize(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ));
		LibfabricConnection * connection = new LibfabricConnection(&domain, false);
		connection->postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection->joinServer();
	});
	server.join();
	client.join();
	ASSERT_TRUE(gotConnection);
}

/****************************************************/
TEST(TestConnection, message)
{
	bool gotConnection = false;
	bool gotMessage = false;
	bool sendMessage = false;

	//server
	std::thread server([&gotConnection, &gotMessage]{
		LibfabricDomain domain("127.0.0.1", "8444", true);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection = true;
		});
		while (!gotConnection) 
			connection.poll(false);
		connection.registerHook(IOC_LF_MSG_PING, [&connection, &gotMessage](int clientId, size_t id, void * buffer) {
			gotMessage = true;
			connection.repostRecive(id);
			return true;
		});
		connection.poll(true);
	});

	//client
	std::thread client([&sendMessage]{
		LibfabricDomain domain("127.0.0.1", "8444", false);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		//send message
		LibfabricMessage msg;
		msg.header.type = IOC_LF_MSG_PING;
		msg.header.clientId = connection.getClientId();
		connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, [&sendMessage](LibfabricPostAction*action){
			sendMessage = true;
			return true;
		});
		connection.poll(true);
	});

	//join
	server.join();
	client.join();

	//check
	ASSERT_TRUE(gotConnection);
	ASSERT_TRUE(gotMessage);
	ASSERT_TRUE(sendMessage);
}

/****************************************************/
TEST(TestConnection, rdma)
{
	bool gotConnection = false;
	bool gotRdmaRead = false;
	bool gotRdmaWrite = false;
	bool ready = false;
	bool canExit = false;
	Iov iov;

	//alloc
	const size_t size = 1024*1024;
	void * ptrServer1 = malloc(size);
	void * ptrServer2 = malloc(size);
	void * ptrClient = malloc(size);

	//ini
	memset(ptrServer1, 1, size);

	//server
	std::thread server([&iov, &canExit, &ready, &gotConnection, &gotRdmaRead, &gotRdmaWrite, ptrServer1, ptrServer2, ptrClient]{
		int clientId = -1;
		LibfabricDomain domain("127.0.0.1", "8448", true);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&clientId, &gotConnection](int id) {
			gotConnection = true;
			clientId = id;
		});

		//register
		Iov iovLocal1 = domain.registerSegment(ptrServer1, size, true, true, false);
		Iov iovLocal2 = domain.registerSegment(ptrServer2, size, true, true, false);

		//ready
		while(!gotConnection)
			connection.poll(false);
		while(!ready){};

		//do write
		connection.rdmaWrite(clientId, ptrServer1, iov.addr, iov.key, size, [&gotRdmaWrite](LibfabricPostAction*action){
			gotRdmaWrite = true;
			return true;
		});
		connection.poll(true);

		//do read
		connection.rdmaRead(clientId, ptrServer2, iov.addr, iov.key, size, [&gotRdmaRead](LibfabricPostAction*action){
			gotRdmaRead = true;
			return true;
		});
		connection.poll(true);

		//clear
		domain.unregisterSegment(ptrServer1, size);
		domain.unregisterSegment(ptrServer2, size);

		//exxit
		canExit = true;
	});

	//client
	std::thread client([&ready, &canExit, &iov, &gotRdmaWrite, &gotRdmaRead, ptrClient]{
		LibfabricDomain domain("127.0.0.1", "8448", false);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		
		//register memory
		iov = domain.registerSegment(ptrClient, size, true, true, false);
		ready = true;

		//wait
		while (!canExit)
			connection.poll(false);

		//clean
		domain.unregisterSegment(ptrClient, size);
	});

	//join
	server.join();
	client.join();

	//check
	ASSERT_TRUE(gotConnection);
	ASSERT_TRUE(gotRdmaRead);
	ASSERT_TRUE(gotRdmaWrite);

	//check
	for (int i = 0 ; i < size ; i++) {
		ASSERT_EQ(1, ((char*)ptrClient)[i]) << i;
		ASSERT_EQ(1, ((char*)ptrServer1)[i]) << i;
		ASSERT_EQ(1, ((char*)ptrServer2)[i]) << i;
	}
}
