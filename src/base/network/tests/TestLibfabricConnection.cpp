/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//gtest
#include <gtest/gtest.h>
//std
#include <thread>
#include <cstring>
//linux
#include <sys/uio.h>
//local
#include "../LibfabricConnection.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
//spawn a server and try to connect a client on it.
TEST(TestLibfabricConnection, connect)
{
	// to check
	bool gotConnection = false;

	//spawn a server in a thread
	std::thread server([&gotConnection]{
		LibfabricDomain domain("127.0.0.1", "8444", true);
		domain.setMsgBuffeSize(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_WRITE));
		LibfabricConnection connection(&domain, false);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection = true;
		});

		//wait first connection then exit loop
		while (!gotConnection)
			connection.poll(false);
	});

	//spawn a client in a thread
	std::thread client([]{
		LibfabricDomain domain("127.0.0.1", "8444", false);
		domain.setMsgBuffeSize(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ));
		LibfabricConnection * connection = new LibfabricConnection(&domain, false);
		connection->postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection->joinServer();
	});

	//wait them
	server.join();
	client.join();

	//check
	ASSERT_TRUE(gotConnection);
}

/****************************************************/
// Connect and client send a message.
TEST(TestLibfabricConnection, message)
{
	bool gotConnection = false;
	bool gotMessage = false;
	bool sendMessage = false;

	//server
	std::thread server([&gotConnection, &gotMessage]{
		LibfabricDomain domain("127.0.0.1", "8446", true);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection = true;
		});

		//wait connection
		while (!gotConnection) 
			connection.poll(false);

		//register hook
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, int clientId, size_t id, void * buffer) {
			gotMessage = true;
			connection->repostRecive(id);
			//say to unblock the poll(true) loop when return
			return LF_WAIT_LOOP_UNBLOCK;
		});

		//poll unit get message
		connection.poll(true);
	});

	//client
	std::thread client([&sendMessage]{
		LibfabricDomain domain("127.0.0.1", "8446", false);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		//send message
		LibfabricMessage msg;
		memset(&msg, 0, sizeof(msg));
		connection.fillProtocolHeader(msg.header, IOC_LF_MSG_PING);
		connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, [&sendMessage](){
			sendMessage = true;
			//say to unblock the poll(true) loop when return
			return LF_WAIT_LOOP_UNBLOCK;
		});
		//poll unit message to be sent
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
// Connect and client send a message.
TEST(TestLibfabricConnection, pollMessage)
{
	bool gotConnection = false;
	bool gotMessage = false;
	bool sendMessage = false;

	//server
	std::thread server([&gotConnection, &gotMessage]{
		LibfabricDomain domain("127.0.0.1", "8476", true);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection = true;
		});

		//wait connection
		while (!gotConnection) 
			connection.poll(false);

		//poll message
		LibfabricClientMessage message;
		bool status = connection.pollMessage(message, IOC_LF_MSG_PING);
		if (status && message.message->header.type == IOC_LF_MSG_PING)
			gotMessage = true;
	});

	//client
	std::thread client([&sendMessage]{
		LibfabricDomain domain("127.0.0.1", "8476", false);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		//send message
		LibfabricMessage msg;
		memset(&msg, 0, sizeof(msg));
		connection.fillProtocolHeader(msg.header, IOC_LF_MSG_PING);
		connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, [&sendMessage](){
			sendMessage = true;
			//say to unblock the poll(true) loop when return
			return LF_WAIT_LOOP_UNBLOCK;
		});
		//poll unit message to be sent
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
// check rdma data transmission (read/write).
TEST(TestLibfabricConnection, rdma)
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
		domain.registerSegment(ptrServer1, size, true, true, false);
		domain.registerSegment(ptrServer2, size, true, true, false);

		//ready
		while(!gotConnection)
			connection.poll(false);
		while(!ready){};

		//do write
		connection.rdmaWrite(clientId, ptrServer1, iov.addr, iov.key, size, [&gotRdmaWrite](){
			gotRdmaWrite = true;
			return LF_WAIT_LOOP_UNBLOCK;
		});
		connection.poll(true);

		//do read
		connection.rdmaRead(clientId, ptrServer2, iov.addr, iov.key, size, [&gotRdmaRead](){
			gotRdmaRead = true;
			return LF_WAIT_LOOP_UNBLOCK;
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
	for (size_t i = 0 ; i < size ; i++) {
		ASSERT_EQ(1, ((char*)ptrClient)[i]) << i;
		ASSERT_EQ(1, ((char*)ptrServer1)[i]) << i;
		ASSERT_EQ(1, ((char*)ptrServer2)[i]) << i;
	}
}

/****************************************************/
// Test rdma transmission with multiple segments in scatter/gather mode.
TEST(TestLibfabricConnection, rdmav)
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
		LibfabricDomain domain("127.0.0.1", "8450", true);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&clientId, &gotConnection](int id) {
			gotConnection = true;
			clientId = id;
		});

		//register
		domain.registerSegment(ptrServer1, size, true, true, false);
		domain.registerSegment(ptrServer2, size, true, true, false);

		//iocs
		struct iovec iov1[2] = {{ptrServer1, size/2},{(char*)ptrServer1+size/2, size/2}};
		struct iovec iov2[2] = {{ptrServer2, size/2},{(char*)ptrServer2+size/2, size/2}};

		//ready
		while(!gotConnection)
			connection.poll(false);
		while(!ready){};

		//do write
		connection.rdmaWritev(clientId, iov1, 2, iov.addr, iov.key, [&gotRdmaWrite](){
			gotRdmaWrite = true;
			return LF_WAIT_LOOP_UNBLOCK;
		});
		connection.poll(true);

		//do read
		connection.rdmaReadv(clientId, iov2, 2, iov.addr, iov.key, [&gotRdmaRead](){
			gotRdmaRead = true;
			return LF_WAIT_LOOP_UNBLOCK;
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
		LibfabricDomain domain("127.0.0.1", "8450", false);
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
	for (size_t i = 0 ; i < size ; i++) {
		ASSERT_EQ(1, ((char*)ptrClient)[i]) << i;
		ASSERT_EQ(1, ((char*)ptrServer1)[i]) << i;
		ASSERT_EQ(1, ((char*)ptrServer2)[i]) << i;
	}
}

/****************************************************/
// Test authentication message, ok.
TEST(TestLibfabricConnection, message_auth_ok)
{
	bool gotConnection = false;
	bool gotMessage = false;
	bool sendMessage = false;

	//server
	std::thread server([&gotConnection, &gotMessage]{
		LibfabricDomain domain("127.0.0.1", "8455", true);
		LibfabricConnection connection(&domain, false);
		connection.getClientRegistry().registerClient(10, 123456789);
		connection.setCheckClientAuth(true);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection = true;
		});
		while (!gotConnection) 
			connection.poll(false);
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, int clientId, size_t id, void * buffer) {
			gotMessage = true;
			connection->repostRecive(id);
			return LF_WAIT_LOOP_UNBLOCK;
		});
		connection.poll(true);
	});

	//client
	std::thread client([&sendMessage]{
		LibfabricDomain domain("127.0.0.1", "8455", false);
		LibfabricConnection connection(&domain, false);
		connection.setTcpClientInfos(10, 123456789);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		//send message
		LibfabricMessage msg;
		memset(&msg, 0, sizeof(msg));
		connection.fillProtocolHeader(msg.header, IOC_LF_MSG_PING);
		connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, [&sendMessage](){
			sendMessage = true;
			return LF_WAIT_LOOP_UNBLOCK;
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
// Test authentication failure.
TEST(TestLibfabricConnection, message_auth_not_ok)
{
	bool gotConnection = false;
	bool gotMessage = false;
	bool gotError = false;
	bool sendMessage = false;

	//server
	std::thread server([&gotConnection, &gotMessage]{
		LibfabricDomain domain("127.0.0.1", "8465", true);
		LibfabricConnection connection(&domain, false);
		connection.getClientRegistry().registerClient(10, 123456789);
		connection.setCheckClientAuth(true);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection = true;
		});
		while (!gotConnection) 
			connection.poll(false);
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, int clientId, size_t id, void * buffer) {
			gotMessage = true;
			connection->repostRecive(id);
			return LF_WAIT_LOOP_UNBLOCK;
		});
		connection.poll(true);
	});

	//client
	std::thread client([&sendMessage, &gotError]{
		LibfabricDomain domain("127.0.0.1", "8465", false);
		LibfabricConnection connection(&domain, false);
		connection.setTcpClientInfos(10, 123);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		//on error
		connection.setOnBadAuth([&gotError](){
			gotError = true;
			return LF_WAIT_LOOP_UNBLOCK;
		});
		//send message
		LibfabricMessage msg;
		connection.fillProtocolHeader(msg.header, IOC_LF_MSG_PING);
		connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, [&sendMessage](){
			sendMessage = true;
			return LF_WAIT_LOOP_UNBLOCK;
		});
		connection.poll(true);
		//wait reception
		connection.poll(true);
	});

	//join
	server.join();
	client.join();

	//check
	EXPECT_TRUE(gotConnection);
	EXPECT_FALSE(gotMessage);
	EXPECT_TRUE(gotError);
	EXPECT_TRUE(sendMessage);
}

/****************************************************/
// Connect and client send a message.
TEST(TestLibfabricConnection, broadcastErrrorMessage)
{
	int gotConnection = 0;
	int gotErrorMessage = 0;

	//server
	std::thread server([&gotConnection]{
		LibfabricDomain domain("127.0.0.1", "8446", true);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection++;
		});

		//wait connection
		while (gotConnection < 2) 
			connection.poll(false);

		//send error and exit
		domain.setMsgBuffeSize(1024*1024);
		connection.broadcastErrrorMessage("This is a test error !");
	});

	//client
	std::thread client1([&gotErrorMessage]{
		LibfabricDomain domain("127.0.0.1", "8446", false);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		//hook
		connection.registerHook(IOC_LF_MSG_FATAL_ERROR, [&gotErrorMessage](LibfabricConnection * connection, int clientId, size_t bufferId, void * buffer){
			gotErrorMessage++;
			return LF_WAIT_LOOP_UNBLOCK;
		});
		//poll unit message to be sent
		connection.poll(true);
	});

	//client
	std::thread client2([&gotErrorMessage]{
		LibfabricDomain domain("127.0.0.1", "8446", false);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		//hook
		connection.registerHook(IOC_LF_MSG_FATAL_ERROR, [&gotErrorMessage](LibfabricConnection * connection, int clientId, size_t bufferId, void * buffer){
			gotErrorMessage++;
			return LF_WAIT_LOOP_UNBLOCK;
		});
		//poll unit message to be sent
		connection.poll(true);
	});

	//join
	server.join();
	client1.join();
	client2.join();

	//check
	ASSERT_EQ(2, gotConnection);
	ASSERT_EQ(2, gotErrorMessage);
}
