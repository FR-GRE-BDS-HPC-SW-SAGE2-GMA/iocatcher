/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <unistd.h>
#include <cstring>
#include <random>
#include <cassert>
#include "Server.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
ServerStats::ServerStats(void)
{
	this->readSize = 0;
	this->writeSize = 0;
}

/****************************************************/
Server::Server(const Config * config, const std::string & port)
{
	//check
	assert(config != NULL);

	//setup
	this->config = config;
	this->pollRunning = false;
	this->statsRunning = false;
	//this->setOnClientConnect([](int id){});

	//setup tcp server
	int tcpPort = atoi(port.c_str()) + 1;
	printf("Server on port %s/%d\n", port.c_str(), tcpPort);
	this->setupTcpServer(tcpPort, tcpPort);

	//setup domain
	this->domain = new LibfabricDomain(config->listenIP, port, config->activePolling);
	this->domain->setMsgBuffeSize(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ));

	//establish connections
	this->connection = new LibfabricConnection(this->domain, false);
	this->connection->postRecives(1024*1024, 64);
	if (config->clientAuth)
		this->connection->setCheckClientAuth(true);

	//create container
	this->container = new Container(domain, 8*1024*1024);
	
	//register actions
	this->setupPingPong();
	this->setupObjFlush();
	this->setupObjRead();
	this->setupObjWrite();
	this->setupObjFlush();
	this->setupObjCreate();
	this->setupObjRangeRegister();
	this->setupObjUnregisterRange();
}

/****************************************************/
Server::~Server(void)
{
	this->stop();
	delete this->container;
	delete this->connection;
	delete this->domain;
	delete this->tcpServer;
}

/****************************************************/
void Server::setupTcpServer(int port, int maxport)
{
	//create server
	if (config->clientAuth)
		this->tcpServer = new TcpServer(port, maxport, "conntrack");
	else
		this->tcpServer = new TcpServer(port, maxport, "noauth");

	//server loop
	this->tcpServerThread = std::thread([this](){
		//vars
		int cntConnections = 0;

		//random generator
		std::random_device rd;
		std::mt19937_64 rdGenerator(rd());
		std::uniform_int_distribution<uint64_t> distribution;

		//start loop
		this->tcpServer->loop([this, &distribution, &rdGenerator, &cntConnections](uint64_t* id, uint64_t* key, TcpClientInfo* client){
			*id = cntConnections++;
			*key = distribution(rdGenerator);
			//printf("=======> ID=%lu, KEY=%lu\n", *id, *key);
			this->onClientConnect(*id, *key);
		},[this](TcpClientInfo* client){
			this->onClientDisconnect(client->id);
		});
	});
}

/****************************************************/
void Server::setOnClientConnect(std::function<void(int id)> handler)
{
	this->connection->setHooks(handler);
}

/****************************************************/
void Server::poll(void)
{
	this->pollRunning = true;
	while(this->pollRunning)
		this->connection->poll(false);
	this->pollRunning = true;
}

/****************************************************/
void Server::startStatsThread(void)
{
	this->statsRunning = true;
	this->statThread = std::thread([this]{
		while (this->statsRunning) {
			sleep(1);
			printf("Read: %g GB/s, Write: %g GB/s\n", (double)this->stats.readSize/1.0/1024.0/1024.0/1024.0, (double) this->stats.writeSize/1.0/1024.0/1024.0/1024.0);
			this->stats.readSize = 0;
			this->stats.writeSize = 0;
		}
	});
}

/****************************************************/
void Server::stop(void)
{
	//wait poll
	if (this->pollRunning) {
		this->pollRunning = false;
		while(this->pollRunning == false) {};
		this->pollRunning = false;
	}

	//stop stats thread
	if (this->statsRunning) {
		this->statsRunning = false;
		this->statThread.join();
	}

	//stop tcp server
	if (this->tcpServer != NULL) {
		this->tcpServer->stop();
		this->tcpServer = NULL;
		this->tcpServerThread.join();
	}
}

/****************************************************/
void Server::setupPingPong(void)
{
	//rma
	char * rmaBuffer = new char[TEST_RDMA_SIZE];
	connection->getDomain().registerSegment(rmaBuffer, TEST_RDMA_SIZE, true, true, false);

	//register hook
	connection->registerHook(IOC_LF_MSG_PING, [this, rmaBuffer](int clientId, size_t id, void * buffer) {
		//printf
		//printf("Get 10 %d\n", clientId);

		//do rdma read
		//LibfabricMessage * clientMessage = (LibfabricMessage *)buffer;
		//connection.rdmaWrite(clientId, rmaBuffer, clientMessage->data.iov.addr, clientMessage->data.iov.key, TEST_RDMA_SIZE, new LibfabricPostActionFunction([&connection, clientId](void){
			//send open
			LibfabricMessage * msg = new LibfabricMessage;
			msg->header.type = IOC_LF_MSG_PONG;
			msg->header.clientId = 0;

			this->connection->sendMessage(msg, sizeof (*msg), clientId, [msg](void){
				delete msg;
				return false;
			});
			
		//	return false;
		//}));

		//republish
		this->connection->repostRecive(id);

		return false;
	});
}

/****************************************************/
void Server::setupObjFlush(void)
{
	//register hook
	this->connection->registerHook(IOC_LF_MSG_OBJ_FLUSH, [this](int clientId, size_t id, void * buffer) {
		//infos
		LibfabricMessage * clientMessage = (LibfabricMessage*)buffer;

		//printf
		//printf("Get flush object %ld:%ld %lu->%lu\n", clientMessage->data.objFlush.high, clientMessage->data.objFlush.low, clientMessage->data.objFlush.offset, clientMessage->data.objFlush.size);

		//flush object
		Object & object = this->container->getObject(clientMessage->data.objFlush.high, clientMessage->data.objFlush.low);
		int ret = object.flush(clientMessage->data.objFlush.offset, clientMessage->data.objFlush.size);

		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_FLUSH_ACK;
		msg->header.clientId = clientId;
		msg->data.response.status = ret;

		this->connection->sendMessage(msg, sizeof (*msg), clientId, [msg](void){
			delete msg;
			return false;
		});

		//republish
		this->connection->repostRecive(id);

		return false;
	});
}

/****************************************************/
void Server::setupObjRangeRegister(void)
{
	//register hook
	this->connection->registerHook(IOC_LF_MSG_OBJ_RANGE_REGISTER, [this](int clientId, size_t id, void * buffer) {
		//infos
		LibfabricMessage * clientMessage = (LibfabricMessage*)buffer;

		//get object
		Object & object = this->container->getObject(clientMessage->data.registerRange.high, clientMessage->data.registerRange.low);
		ConsistencyTracker & tracker = object.getConsistencyTracker();

		//check
		int status = 0;
		ConsistencyAccessMode mode = CONSIST_ACCESS_MODE_READ;
		if (clientMessage->data.registerRange.write)
			mode = CONSIST_ACCESS_MODE_WRITE;
		if (this->config->consistencyCheck)
			status = tracker.registerRange(0, clientMessage->data.registerRange.offset, clientMessage->data.registerRange.size, mode);

		//return message
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_RANGE_REGISTER_ACK;
		msg->header.clientId = clientId;
		msg->data.response.status = status;
		msg->data.response.msgHasData = false;

		this->connection->sendMessage(msg, sizeof (*msg), clientId, [msg](){
			delete msg;
			return false;
		});

		//republish
		this->connection->repostRecive(id);

		//
		return false;
	});
}

/****************************************************/
void Server::setupObjUnregisterRange(void)
{
	//register hook
	this->connection->registerHook(IOC_LF_MSG_OBJ_RANGE_UNREGISTER, [this](int clientId, size_t id, void * buffer) {
		//infos
		LibfabricMessage * clientMessage = (LibfabricMessage*)buffer;

		//get object
		Object & object = this->container->getObject(clientMessage->data.registerRange.high, clientMessage->data.registerRange.low);
		ConsistencyTracker & tracker = object.getConsistencyTracker();

		//extract
		LibfabricUnregisterRange &data = clientMessage->data.unregisterRange;

		//check
		int status = 0;
		ConsistencyAccessMode mode = CONSIST_ACCESS_MODE_READ;
		if (data.write)
			mode = CONSIST_ACCESS_MODE_WRITE;
		if (this->config->consistencyCheck)
			if (!tracker.unregisterRange(0, data.id, data.offset, data.size, mode))
				status = -1;

		//return message
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_RANGE_UNREGISTER_ACK;
		msg->header.clientId = clientId;
		msg->data.response.status = status;
		msg->data.response.msgHasData = false;

		this->connection->sendMessage(msg, sizeof (*msg), clientId, [msg](){
			delete msg;
			return false;
		});

		//republish
		this->connection->repostRecive(id);

		//
		return false;
	});
}

/****************************************************/
void Server::setupObjCreate(void)
{
	//register hook
	this->connection->registerHook(IOC_LF_MSG_OBJ_CREATE, [this](int clientId, size_t id, void * buffer) {
		//infos
		LibfabricMessage * clientMessage = (LibfabricMessage*)buffer;

		//printf
		printf("Get create object %ld:%ld\n", clientMessage->data.objCreate.high, clientMessage->data.objCreate.low);

		//create object
		Object & object = this->container->getObject(clientMessage->data.objFlush.high, clientMessage->data.objFlush.low);
		int ret = object.create();

		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_CREATE_ACK;
		msg->header.clientId = clientId;
		msg->data.response.status = ret;

		this->connection->sendMessage(msg, sizeof (*msg), clientId, [msg](void){
			delete msg;
			return false;
		});

		//republish
		this->connection->repostRecive(id);

		return false;
	});
}

/****************************************************/
iovec * Server::buildIovec(ObjectSegmentList & segments, size_t offset, size_t size)
{
	//compute intersection
	for (auto & it : segments) {
		if (it.offset < offset) {
			int delta = offset - it.offset;
			it.ptr += delta;
			it.offset += delta;
			it.size -= delta;
		}
		if (it.offset + it.size > offset + size) {
			int delta = it.offset + it.size - (offset + size);
			it.size -= delta;
		}
	}

	//build iov
	struct iovec * iov = new iovec[segments.size()];
	int cnt = 0;
	for (auto & it : segments) {
		iov[cnt].iov_base = it.ptr;
		iov[cnt].iov_len = it.size;
		cnt++;
	}

	return iov;
}

/****************************************************/
void Server::objRdmaPushToClient(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	if (segments.size() == 2)
		segments.pop_back();
	//build iovec
	iovec * iov = buildIovec(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	size_t size = clientMessage->data.objReadWrite.size;

	//dispaly
	//printf("IOV SIZE %lu (%zu, %zu)\n", segments.size(), clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	//int cnt = 0;
	//for (auto it : segments)
	//	printf("-> %p %zu %zu (%p %zu)\n", it.ptr, it.offset, it.size, iov[cnt].iov_base, iov[cnt++].iov_len);

	//emit rdma write vec & implement callback
	this->connection->rdmaWritev(clientId, iov, segments.size(), clientMessage->data.objReadWrite.iov.addr, clientMessage->data.objReadWrite.iov.key, [size, this, clientId](void){
		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
		msg->header.clientId = 0;
		msg->data.response.msgHasData = false;
		msg->data.response.msgDataSize = 0;
		msg->data.response.status = 0;

		//stats
		this->stats.readSize += size;

		//send ack message
		this->connection->sendMessage(msg, sizeof (*msg), clientId, [msg](void){
			delete msg;
			return false;
		});
		
		return false;
	});

	//remove temp
	delete [] iov;
}

/****************************************************/
void Server::objEagerPushToClient(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//size
	size_t dataSize = clientMessage->data.objReadWrite.size;

	//send open
	//char * buffer = new char[sizeof(LibfabricMessage) + dataSize];
	LibfabricMessage * msg = (LibfabricMessage *)this->connection->getDomain().getMsgBuffer();
	//LibfabricMessage * msg = (LibfabricMessage*)buffer;
	msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
	msg->header.clientId = 0;
	msg->data.response.msgDataSize = dataSize;
	msg->data.response.msgHasData = true;
	msg->data.response.status = 0;

	//get base pointer
	char * data = (char*)(msg + 1);

	//copy data
	size_t cur = 0;
	for (auto segment : segments) {
		//compute copy size to stay in data limits
		size_t copySize = segment.size;
		if (cur + copySize > dataSize)
			copySize = dataSize - cur;

		//copy
		memcpy(data + cur, segment.ptr, copySize);

		//progress
		cur += copySize;
	}

	//stats
	this->stats.readSize += dataSize;

	//send ack message
	this->connection->sendMessage(msg, sizeof (*msg) + dataSize, clientId, [this, msg](void){
		this->connection->getDomain().retMsgBuffer(msg);
		return false;
	});
}

/****************************************************/
void Server::setupObjRead(void)
{
	//register hook
	this->connection->registerHook(IOC_LF_MSG_OBJ_READ, [this](int clientId, size_t id, void * buffer) {
		//printf
		//printf("Get OBJ_READ %d\n", clientId);

		//do rdma write on remote segment to send data to reader
		LibfabricMessage * clientMessage = (LibfabricMessage *)buffer;

		//get buffers from object
		Object & object = this->container->getObject(clientMessage->data.objReadWrite.high, clientMessage->data.objReadWrite.low);
		ObjectSegmentList segments;
		object.getBuffers(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);

		//eager or rdma
		if (clientMessage->data.objReadWrite.size <= IOC_EAGER_MAX_READ) {
			objEagerPushToClient(clientId, clientMessage, segments);
		} else {
			objRdmaPushToClient(clientId, clientMessage, segments);
		}

		//republish
		this->connection->repostRecive(id);

		return false;
	});
}

/****************************************************/
void Server::objRdmaFetchFromClient(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//build iovec
	iovec * iov = buildIovec(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	size_t size = clientMessage->data.objReadWrite.size;

	//emit rdma write vec & implement callback
	this->connection->rdmaReadv(clientId, iov, segments.size(), clientMessage->data.objReadWrite.iov.addr, clientMessage->data.objReadWrite.iov.key, [size, this, clientId, iov](void){
		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
		msg->header.clientId = 0;
		msg->data.response.msgHasData = false;
		msg->data.response.msgDataSize = 0;
		msg->data.response.status = 0;

		//stats
		this->stats.writeSize += size;

		//send ack message
		this->connection->sendMessage(msg, sizeof (*msg), clientId, [msg](void){
			delete msg;
			return false;
		});
		
		return false;
	});

	//remove temp
	delete [] iov;
}

/****************************************************/
void Server::objEagerExtractFromMessage(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//get base pointer
	char * data = (char*)(clientMessage + 1);

	//copy data
	size_t cur = 0;
	size_t dataSize = clientMessage->data.objReadWrite.size;
	for (auto segment : segments) {
		//compute copy size to stay in data limits
		size_t copySize = segment.size;
		if (cur + copySize > dataSize)
			copySize = dataSize - cur;

		//copy
		memcpy(segment.ptr, data + cur, copySize);

		//progress
		cur += copySize;
	}

	//send open
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
	msg->header.clientId = 0;
	msg->data.response.status = 0;

	//stats
	this->stats.writeSize += cur;

	//send ack message
	this->connection->sendMessage(msg, sizeof (*msg), clientId, [msg](void){
		delete msg;
		return false;
	});
}

/****************************************************/
void Server::setupObjWrite(void)
{
	//register hook
	this->connection->registerHook(IOC_LF_MSG_OBJ_WRITE, [this](int clientId, size_t id, void * buffer) {
		//printf
		//printf("Get OBJ_READ %d\n", clientId);

		//do rdma write on remote segment to send data to write
		LibfabricMessage * clientMessage = (LibfabricMessage *)buffer;

		//get buffers from object
		Object & object = this->container->getObject(clientMessage->data.objReadWrite.high, clientMessage->data.objReadWrite.low);
		ObjectSegmentList segments;
		object.getBuffers(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size, false);

		//eager or rdma
		if (clientMessage->data.objReadWrite.msgHasData) {
			objEagerExtractFromMessage(clientId, clientMessage, segments);
		} else {
			objRdmaFetchFromClient(clientId, clientMessage, segments);
		}

		//mark dirty
		object.markDirty(clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);

		//republish
		this->connection->repostRecive(id);

		return false;
	});
}

/****************************************************/
void Server::onClientConnect(uint64_t id, uint64_t key)
{
	printf("Client connect ID=%lu, key=%lu\n", id, key);
	connection->getClientRegistry().registerClient(id, key);
}

/****************************************************/
void Server::onClientDisconnect(uint64_t id)
{
	printf("Client disconnect ID=%lu\n", id);
	connection->getClientRegistry().disconnectClient(id);
	container->onClientDisconnect(id);
}
