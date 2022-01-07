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
//helper function to quickly build a client connected to a server and play exchanges
void clientServer(std::function<void(LibfabricConnection & connection,int clientId)> serverAction, std::function<void(LibfabricConnection & connection)> clientAction)
{
	bool gotConnection = false;
	volatile bool serverReady = false;

	//server
	std::thread server([&gotConnection, &serverReady, &serverAction]{
		LibfabricDomain domain("127.0.0.1", "8446", true);
		domain.setMsgBuffeSize(1024*1024);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(1024*1024, 64);
		int clientId = 0;
		connection.setHooks([&gotConnection,&clientId](int id) {
			gotConnection = true;
			clientId = id;
		});

		//wait connection
		serverReady = true;
		while (!gotConnection) 
			connection.poll(false);

		//call action
		serverAction(connection, clientId);
	});

	//wait server
	while(!serverReady){};

	//client
	std::thread client([&clientAction]{
		LibfabricDomain domain("127.0.0.1", "8446", false);
		domain.setMsgBuffeSize(1024*1024);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		clientAction(connection);
	});

	//join
	server.join();
	client.join();

	//check
	ASSERT_TRUE(gotConnection);
}

/****************************************************/
//spawn a server and try to connect a client on it.
TEST(TestLibfabricConnection, connect)
{
	//vars
	bool serverOk = false;
	bool clientOk = false;

	//play client server
	clientServer([&serverOk](LibfabricConnection & connection, int clientId){
		//>>>> server <<<<
		serverOk = true;
	},[&clientOk](LibfabricConnection & connection){
		//>>>> client <<<<
		clientOk = true;
	});

	//check
	EXPECT_TRUE(clientOk);
	EXPECT_TRUE(serverOk);
}

/****************************************************/
// Connect and client send a message.
TEST(TestLibfabricConnection, message)
{
	//vars
	bool gotMessage = false;
	bool sendMessage = false;

	//play client server
	clientServer([&gotMessage](LibfabricConnection & connection, int clientId){
		//>>>> server <<<<

		//register hook
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, LibfabricClientRequest & request) {
			gotMessage = true;
			connection->repostReceive(request.msgBufferId);
			//say to unblock the poll(true) loop when return
			return LF_WAIT_LOOP_UNBLOCK;
		});

		//poll until get message
		connection.poll(true);
	},[&sendMessage](LibfabricConnection & connection){
		//>>>> client <<<<

		//send message
		LibfabricMessage msg;
		memset(&msg, 0, sizeof(msg));
		connection.fillProtocolHeader(msg.header, IOC_LF_MSG_PING);
		connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, [&sendMessage](){
			sendMessage = true;
			//say to unblock the poll(true) loop when return
			return LF_WAIT_LOOP_UNBLOCK;
		});
		//poll until message to be sent
		connection.poll(true);
	});
	//check
	ASSERT_TRUE(gotMessage);
	ASSERT_TRUE(sendMessage);
}

/****************************************************/
TEST(TestLibfabricConnection, sendResponse)
{
	bool gotMessage = false;

	//play client server
	clientServer([&gotMessage](LibfabricConnection & connection, int clientId){
		//>>>> server <<<<

		//register hook
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, LibfabricClientRequest & request) {
			//extract & check
			LibfabricResponse & response = request.message->data.response;
			EXPECT_EQ(-1, response.status);

			//end
			gotMessage = true;
			connection->repostReceive(request.msgBufferId);
			//say to unblock the poll(true) loop when return
			return LF_WAIT_LOOP_UNBLOCK;
		});

		//poll unit get message
		connection.poll(true);
	},[](LibfabricConnection & connection){
		//>>>> client <<<<
		connection.sendResponse(IOC_LF_MSG_PING, IOC_LF_SERVER_ID, -1, true);
		connection.poll(true);
	});
}

/****************************************************/
TEST(TestLibfabricConnection, sendResponse_with_data)
{
	bool gotMessage = false;

	//play client server
	clientServer([&gotMessage](LibfabricConnection & connection, int clientId){
		//>>>> server <<<<

		//register hook
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, LibfabricClientRequest & request) {
			//extract & check
			LibfabricResponse & response = request.message->data.response;
			EXPECT_EQ(-1, response.status);
			EXPECT_EQ(6, response.msgDataSize);
			EXPECT_TRUE(response.msgHasData);
			EXPECT_STREQ("hello", request.message->extraData);

			//end
			gotMessage = true;
			connection->repostReceive(request.msgBufferId);
			//say to unblock the poll(true) loop when return
			return LF_WAIT_LOOP_UNBLOCK;
		});

		//poll unit get message
		connection.poll(true);
	},[](LibfabricConnection & connection){
		//>>>> client <<<<
		connection.sendResponse(IOC_LF_MSG_PING, IOC_LF_SERVER_ID, -1, "hello", 6, true);
		connection.poll(true);
	});

	ASSERT_TRUE(gotMessage);
}

/****************************************************/
TEST(TestLibfabricConnection, sendResponse_with_data_multi)
{
	bool gotMessage = false;

	//play client server
	clientServer([&gotMessage](LibfabricConnection & connection, int clientId){
		//>>>> server <<<<

		//register hook
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, LibfabricClientRequest & request) {
			//extract & check
			LibfabricResponse & response = request.message->data.response;
			EXPECT_EQ(-1, response.status);
			EXPECT_EQ(11, response.msgDataSize);
			EXPECT_TRUE(response.msgHasData);
			EXPECT_STREQ("HelloWorld", request.message->extraData);
			gotMessage = true;
			connection->repostReceive(request.msgBufferId);
			//say to unblock the poll(true) loop when return
			return LF_WAIT_LOOP_UNBLOCK;
		});

		//poll unit get message
		connection.poll(true);
	},[](LibfabricConnection & connection){
		//>>>> client <<<<
		//build multi buffers to concat
		char b1[] = "Hello";
		char b2[] = "World";
		LibfabricBuffer buffers[2] = {{b1, 5},{b2,6}};

		//send all in one go
		connection.sendResponse(IOC_LF_MSG_PING, IOC_LF_SERVER_ID, -1, buffers, 2, true);

		//wait
		connection.poll(true);
	});
}

/****************************************************/
// Connect and client send a request.
TEST(TestLibfabricConnection, pollMessage)
{
	//vars
	bool gotMessage = false;
	bool sendMessage = false;

	//play client server
	clientServer([&gotMessage](LibfabricConnection & connection, int clientId){
		//>>>> server <<<<
		//poll message
		LibfabricRemoteResponse remoteResponse;
		bool status = connection.pollMessage(remoteResponse, IOC_LF_MSG_PING);
		if (status && remoteResponse.message->header.msgType == IOC_LF_MSG_PING)
			gotMessage = true;
	},[&sendMessage](LibfabricConnection & connection){
		//>>>> client <<<<
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

	//check
	ASSERT_TRUE(gotMessage);
	ASSERT_TRUE(sendMessage);
}

/****************************************************/
// check rdma data transmission (read/write).
TEST(TestLibfabricConnection, rdma)
{
	bool gotRdmaRead = false;
	bool gotRdmaWrite = false;
	volatile bool ready = false;
	volatile bool canExit = false;
	Iov iov;

	//alloc
	const size_t size = 1024*1024;
	void * ptrServer1 = malloc(size);
	void * ptrServer2 = malloc(size);
	void * ptrClient = malloc(size);

	//ini
	memset(ptrServer1, 1, size);

	//play client server
	clientServer([&iov, &canExit, &ready, &gotRdmaRead, &gotRdmaWrite, ptrServer1, ptrServer2](LibfabricConnection & connection, int clientId){
		//>>>> server <<<<
		while(!ready){};
		
		//register
		LibfabricDomain & domain = connection.getDomain();
		domain.registerSegment(ptrServer1, size, true, true, false);
		domain.registerSegment(ptrServer2, size, true, true, false);

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
	},[&ready, &canExit, &iov, ptrClient](LibfabricConnection & connection){
		//>>>> client <<<<
		
		//register memory
		LibfabricDomain & domain = connection.getDomain();
		iov = domain.registerSegment(ptrClient, size, true, true, false);
		ready = true;

		//wait
		while (!canExit)
			connection.poll(false);

		//clean
		domain.unregisterSegment(ptrClient, size);
	});

	//check
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
	bool gotRdmaRead = false;
	bool gotRdmaWrite = false;
	volatile bool ready = false;
	volatile bool canExit = false;
	Iov iov;

	//alloc
	const size_t size = 1024*1024;
	void * ptrServer1 = malloc(size);
	void * ptrServer2 = malloc(size);
	void * ptrClient = malloc(size);

	//ini
	memset(ptrServer1, 1, size);

	//play client server
	clientServer([&iov, &canExit, &ready, &gotRdmaRead, &gotRdmaWrite, ptrServer1, ptrServer2](LibfabricConnection & connection, int clientId){
		//>>>> server <<<<

		//register
		LibfabricDomain & domain = connection.getDomain();
		domain.registerSegment(ptrServer1, size, true, true, false);
		domain.registerSegment(ptrServer2, size, true, true, false);

		//iocs
		struct iovec iov1[2] = {{ptrServer1, size/2},{(char*)ptrServer1+size/2, size/2}};
		struct iovec iov2[2] = {{ptrServer2, size/2},{(char*)ptrServer2+size/2, size/2}};

		//ready
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
	},[&ready, &canExit, &iov, ptrClient](LibfabricConnection & connection){
		//>>>>> client <<<<

		//register memory
		LibfabricDomain & domain = connection.getDomain();
		iov = domain.registerSegment(ptrClient, size, true, true, false);
		ready = true;

		//wait
		while (!canExit)
			connection.poll(false);

		//clean
		domain.unregisterSegment(ptrClient, size);
	});

	//check
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
	volatile bool serverReady = false;

	//server
	std::thread server([&gotConnection, &gotMessage, &serverReady]{
		LibfabricDomain domain("127.0.0.1", "8455", true);
		domain.setMsgBuffeSize(1024*1024);
		LibfabricConnection connection(&domain, false);
		connection.getClientRegistry().registerClient(10, 123456789);
		connection.setCheckClientAuth(true);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection = true;
		});
		serverReady = true;
		while (!gotConnection) 
			connection.poll(false);
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, LibfabricClientRequest & request) {
			gotMessage = true;
			connection->repostReceive(request.msgBufferId);
			return LF_WAIT_LOOP_UNBLOCK;
		});
		connection.poll(true);
	});

	//wait server
	while(!serverReady) {};

	//client
	std::thread client([&sendMessage]{
		LibfabricDomain domain("127.0.0.1", "8455", false);
		domain.setMsgBuffeSize(1024*1024);
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
	volatile bool serverReady = false;

	//server
	std::thread server([&gotConnection, &gotMessage, &serverReady]{
		LibfabricDomain domain("127.0.0.1", "8465", true);
		domain.setMsgBuffeSize(1024*1024);
		LibfabricConnection connection(&domain, false);
		connection.getClientRegistry().registerClient(10, 123456789);
		connection.setCheckClientAuth(true);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection = true;
		});
		serverReady = true;
		while (!gotConnection) 
			connection.poll(false);
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, LibfabricClientRequest & request) {
			gotMessage = true;
			connection->repostReceive(request.msgBufferId);
			return LF_WAIT_LOOP_UNBLOCK;
		});
		connection.poll(true);
	});

	//wait server
	while(!serverReady) {};

	//client
	std::thread client([&sendMessage, &gotError]{
		LibfabricDomain domain("127.0.0.1", "8465", false);
		domain.setMsgBuffeSize(1024*1024);
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
// Connect and client send a request.
TEST(TestLibfabricConnection, broadcastErrrorMessage)
{
	volatile int gotConnection = 0;
	volatile bool gotErrorMessage1 = false;
	volatile bool gotErrorMessage2 = false;
	volatile bool serverReady = false;

	//server
	std::thread server([&gotConnection, &serverReady]{
		LibfabricDomain domain("127.0.0.1", "8446", true);
		domain.setMsgBuffeSize(1024*1024);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(1024*1024, 64);
		connection.setHooks([&gotConnection](int id) {
			gotConnection++;
		});

		//wait connection
		serverReady = true;
		while (gotConnection < 2) 
			connection.poll(false);

		//send error and exit
		domain.setMsgBuffeSize(1024*1024);
		connection.broadcastErrrorMessage("This is a test error !");
	});

	//wait server
	while(!serverReady) {};

	//client
	std::thread client1([&gotErrorMessage1]{
		LibfabricDomain domain("127.0.0.1", "8446", false);
		domain.setMsgBuffeSize(1024*1024);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		//hook
		connection.registerHook(IOC_LF_MSG_FATAL_ERROR, [&gotErrorMessage1](LibfabricConnection * connection, LibfabricClientRequest & request){
			gotErrorMessage1 = true;
			connection->repostReceive(request.msgBufferId);
			return LF_WAIT_LOOP_UNBLOCK;
		});
		//poll unit message to be sent
		connection.poll(true);
	});

	//client
	std::thread client2([&gotErrorMessage2]{
		LibfabricDomain domain("127.0.0.1", "8446", false);
		domain.setMsgBuffeSize(1024*1024);
		LibfabricConnection connection(&domain, false);
		connection.postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
		connection.joinServer();
		//hook
		connection.registerHook(IOC_LF_MSG_FATAL_ERROR, [&gotErrorMessage2](LibfabricConnection * connection, LibfabricClientRequest & request){
			gotErrorMessage2 = true;
			connection->repostReceive(request.msgBufferId);
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
	ASSERT_TRUE(gotErrorMessage1);
	ASSERT_TRUE(gotErrorMessage1);
}

/****************************************************/
TEST(TestLibfabricConnection, sendMessage_serialize)
{
	bool gotMessage = false;
	bool sendMessage = false;

	//play client server
	clientServer([&gotMessage](LibfabricConnection & connection, int clientId){
		//>>>> server <<<<

		//register hook
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, LibfabricClientRequest & request) {
			//extract & check
			LibfabricObjCreateInfos & response = request.message->data.objCreate;
			EXPECT_EQ(10, response.objectId.low);
			EXPECT_EQ(20, response.objectId.high);

			//end
			gotMessage = true;
			connection->repostReceive(request.msgBufferId);
			//say to unblock the poll(true) loop when return
			return LF_WAIT_LOOP_UNBLOCK;
		});

		//poll unit get message
		connection.poll(true);
	},[&sendMessage](LibfabricConnection & connection){
		//>>>> client <<<<
		LibfabricObjCreateInfos objCreate = {
			.objectId = {
				.low = 10,
				.high = 20,
			}
		};
		connection.getDomain().setMsgBuffeSize(1024*1024);
		connection.sendMessage(IOC_LF_MSG_PING, IOC_LF_SERVER_ID, objCreate, [&sendMessage](void) {
			sendMessage = true;
			return LF_WAIT_LOOP_UNBLOCK;
		});
		connection.poll(true);
	});

	ASSERT_TRUE(gotMessage);
	ASSERT_TRUE(sendMessage);
}

/****************************************************/
TEST(TestLibfabricConnection, sendMessageNoPollWakeup_serialize)
{
	bool gotMessage = false;
	bool sendMessage = false;

	//play client server
	clientServer([&gotMessage](LibfabricConnection & connection, int clientId){
		//>>>> server <<<<

		//register hook
		connection.registerHook(IOC_LF_MSG_PING, [&gotMessage](LibfabricConnection * connection, LibfabricClientRequest & request) {
			//extract & check
			LibfabricObjCreateInfos & response = request.message->data.objCreate;
			EXPECT_EQ(10, response.objectId.low);
			EXPECT_EQ(20, response.objectId.high);

			//end
			gotMessage = true;
			connection->repostReceive(request.msgBufferId);

			//respond
			connection->sendResponse(IOC_LF_MSG_PONG, request.lfClientId, 0, true);

			//say to unblock the poll(true) loop when return
			return LF_WAIT_LOOP_KEEP_WAITING;
		});

		//poll until get message
		connection.poll(true);
	},[&sendMessage](LibfabricConnection & connection){
		//>>>> client <<<<
		LibfabricObjCreateInfos objCreate = {
			.objectId = {
				.low = 10,
				.high = 20,
			}
		};
		connection.getDomain().setMsgBuffeSize(1024*1024);
		connection.sendMessageNoPollWakeup(IOC_LF_MSG_PING, IOC_LF_SERVER_ID, objCreate);
		sendMessage = true;

		//wait message back
		LibfabricRemoteResonse response;
		connection.pollMessage(response, IOC_LF_MSG_PONG);
	});

	ASSERT_TRUE(gotMessage);
	ASSERT_TRUE(sendMessage);
}
