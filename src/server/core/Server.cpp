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
#include "../hooks/HookPingPong.hpp"
#include "../hooks/HookFlush.hpp"
#include "../hooks/HookRangeRegister.hpp"
#include "../hooks/HookRangeUnregister.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
#define IOC_LF_MAX_RDMA_SEGS 4

/****************************************************/
/** 
 * Constructor of the server stat, it init values to 0. 
**/
ServerStats::ServerStats(void)
{
	this->readSize = 0;
	this->writeSize = 0;
}

/****************************************************/
/**
 * Constructor of the server. It create all sub object and register the hooks for
 * message reception on the libfabric connection.
 * @param config Pointer to the config object.
 * @param port Port to listen on (libfabric port which is tcp port + 1).
**/
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
	this->domain = new LibfabricDomain(config->listenIP, port, true);
	this->domain->setMsgBuffeSize(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ));

	//establish connections
	this->connection = new LibfabricConnection(this->domain, false);
	//this->connection = new LibfabricConnection(this->domain, !config->activePolling);
	this->connection->postRecives(1024*1024, 64);
	if (config->clientAuth)
		this->connection->setCheckClientAuth(true);

	//create container
	this->container = new Container(domain, 8*1024*1024);

	//register hooks
	this->connection->registerHook(IOC_LF_MSG_PING, new HookPingPong());
	this->connection->registerHook(IOC_LF_MSG_OBJ_FLUSH, new HookFlush(this->container));
	this->connection->registerHook(IOC_LF_MSG_OBJ_RANGE_REGISTER, new HookRangeRegister(this->config, this->container));
	this->connection->registerHook(IOC_LF_MSG_OBJ_RANGE_UNREGISTER, new HookRangeUnregister(this->config, this->container));

	//register actions
	this->setupObjRead();
	this->setupObjWrite();
	this->setupObjCreate();
}

/****************************************************/
/**
 * Clean the sub objects.
**/
Server::~Server(void)
{
	this->stop();
	delete this->container;
	delete this->connection;
	delete this->domain;
	delete this->tcpServer;
}

/****************************************************/
/**
 * Setup the TCP server to recive new cilents. It start a new thread
 * to handle the connection. It also provide the handler to assign
 * clientIDs and client key (randomly generated) to auth the clients.
 * @param port Define the TCP port to listen.
 * @param maxport If different than port it allows auto 
 * selecting the first available port in the given range.
**/
void Server::setupTcpServer(int port, int maxport)
{
	//create server
	if (config->clientAuth)
		this->tcpServer = new TcpServer(port, maxport, true);
	else
		this->tcpServer = new TcpServer(port, maxport, false);

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
/**
 * Setup handler on client connection.
 * @param handler Lambda function to be called on client
 * connection so we can print a message.
**/
void Server::setOnClientConnect(std::function<void(int id)> handler)
{
	this->connection->setHooks(handler);
}

/****************************************************/
/**
 * Polling function. it polls until this->pollRunning become false.
**/
void Server::poll(void)
{
	this->pollRunning = true;
	while(this->pollRunning)
		this->connection->poll(false);
	this->pollRunning = true;
}

/****************************************************/
/**
 * Start the statistics thread to print the bandwidths 
 * on the terminal every seconds.
 * It can be stopped by setting statsRunning to false.
**/
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
/**
 * Stop the tcp thread and the polling.
**/
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
		delete this->tcpServer;
		this->tcpServer = NULL;
		this->tcpServerThread.join();
	}
}

/****************************************************/
/**
 * Register the hook for OBJ_CREATE messages and apply the flush on reception, then
 * answer with an ACK message.
**/
void Server::setupObjCreate(void)
{
	//register hook
	this->connection->registerHook(IOC_LF_MSG_OBJ_CREATE, [this](LibfabricConnection * connection, int clientId, size_t id, void * buffer) {
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

		connection->sendMessage(msg, sizeof (*msg), clientId, [msg](void){
			delete msg;
			return LF_WAIT_LOOP_KEEP_WAITING;
		});

		//republish
		connection->repostRecive(id);

		return LF_WAIT_LOOP_KEEP_WAITING;
	});
}

/****************************************************/
/**
 * Build the IOC vector to make scatter/gather RDMA operations.
 * It take a segment list and compute the intersection if not fully matched, 
 * then build the segment list to be used.
 * @param segments The list of object segments to consider.
 * @param offset The base offset of the range to consider.
 * @param size The size of the range to consider.
 * @return An array of iovec struct to be used by libfabric scatter/gather RDMA operations.
 * It need to be freed by the caller with delete.
**/
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
/**
 * Push data to the client via RDMA.
 * @param clientId Define the libfabric client ID.
 * @param clientMessage Pointer the the message requesting the RDMA read operation.
 * @param segments The list of object segments to transfer.
**/
void Server::objRdmaPushToClient(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//build iovec
	iovec * iov = buildIovec(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	size_t size = clientMessage->data.objReadWrite.size;

	//dispaly
	//printf("IOV SIZE %lu (%zu, %zu)\n", segments.size(), clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	//int cnt = 0;
	//for (auto it : segments)
	//	printf("-> %p %zu %zu (%p %zu)\n", it.ptr, it.offset, it.size, iov[cnt].iov_base, iov[cnt++].iov_len);

	//count number of ops
	int  * ops = new int;
	*ops = 0;
	for (size_t i = 0 ; i < segments.size() ; i += IOC_LF_MAX_RDMA_SEGS)
		(*ops)++;

	//loop on all send groups (because LF cannot send more than 256 at same time)
	size_t offset = 0;
	for (size_t i = 0 ; i < segments.size() ; i += IOC_LF_MAX_RDMA_SEGS) {
		//calc cnt
		size_t cnt = segments.size() - i;
		if (cnt > IOC_LF_MAX_RDMA_SEGS)
			cnt = IOC_LF_MAX_RDMA_SEGS;
		//emit rdma write vec & implement callback
		this->connection->rdmaWritev(clientId, iov + i, cnt, (char*)clientMessage->data.objReadWrite.iov.addr + offset, clientMessage->data.objReadWrite.iov.key, [ops,size, this, clientId](void){
			//decrement
			(*ops)--;

			if (*ops == 0) {
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
					return LF_WAIT_LOOP_KEEP_WAITING;
				});

				//clean
				delete ops;
			}
			
			return LF_WAIT_LOOP_KEEP_WAITING;
		});

		//update offset
		for (size_t j = 0 ; j < cnt ; j++)
			offset += iov[i+j].iov_len;
	}

	//remove temp
	delete [] iov;
}

/****************************************************/
/**
 * Push data to the client making an eager communication and adding the data after the response
 * to the client.
 * @param clientId the libfabric client ID to know the connection to be used.
 * @param clientMessage the request from the client to get the required informations.
 * @param segments The list of object segments to be sent.
**/
void Server::objEagerPushToClient(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//size
	size_t dataSize = clientMessage->data.objReadWrite.size;
	size_t baseOffset = clientMessage->data.objReadWrite.offset;

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
		size_t offset = 0;
		if (cur + copySize > dataSize) {
			copySize = dataSize - cur;
		}
		if (baseOffset > segment.offset) {
			offset = baseOffset - segment.offset;
			copySize -= offset;
		}

		//copy
		memcpy(data + cur, segment.ptr + offset, copySize);

		//progress
		cur += copySize;
	}

	//stats
	this->stats.readSize += dataSize;

	//send ack message
	this->connection->sendMessage(msg, sizeof (*msg) + dataSize, clientId, [this, msg](void){
		this->connection->getDomain().retMsgBuffer(msg);
		return LF_WAIT_LOOP_KEEP_WAITING;
	});
}

/****************************************************/
/**
 * Register the hook for OBJ_READ messages and apply the flush on reception, then
 * answer with an ACK message after making an eager (int the response) or an rdam transfer.
**/
void Server::setupObjRead(void)
{
	//register hook
	this->connection->registerHook(IOC_LF_MSG_OBJ_READ, [this](LibfabricConnection * connection, int clientId, size_t id, void * buffer) {
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
		connection->repostRecive(id);

		return LF_WAIT_LOOP_KEEP_WAITING;
	});
}

/****************************************************/
/**
 * Fetch data from the client via RDMA.
 * @param clientId Define the libfabric client ID.
 * @param clientMessage Pointer the the message requesting the RDMA write operation.
 * @param segments The list of object segments to transfer.
**/
void Server::objRdmaFetchFromClient(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//build iovec
	iovec * iov = buildIovec(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	size_t size = clientMessage->data.objReadWrite.size;

	//count number of ops
	int  * ops = new int;
	*ops = 0;
	for (size_t i = 0 ; i < segments.size() ; i += IOC_LF_MAX_RDMA_SEGS)
		(*ops)++;

	//loop on all send groups (because LF cannot send more than 256 at same time)
	size_t offset = 0;
	for (size_t i = 0 ; i < segments.size() ; i += IOC_LF_MAX_RDMA_SEGS) {
		//calc cnt
		size_t cnt = segments.size() - i;
		if (cnt > IOC_LF_MAX_RDMA_SEGS)
			cnt = IOC_LF_MAX_RDMA_SEGS;

		//emit rdma write vec & implement callback
		this->connection->rdmaReadv(clientId, iov + i, cnt, (char*)clientMessage->data.objReadWrite.iov.addr + offset, clientMessage->data.objReadWrite.iov.key, [ops, size, this, clientId, iov](void){
			//decrement
			(*ops)--;

			if (*ops == 0) {
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
					return LF_WAIT_LOOP_KEEP_WAITING;
				});

				//clean
				delete ops;
			}
			
			return LF_WAIT_LOOP_KEEP_WAITING;
		});

		//update offset
		for (size_t j = 0 ; j < cnt ; j++)
			offset += iov[i+j].iov_len;
	}

	//remove temp
	delete [] iov;
}

/****************************************************/
/**
 * Pull data to the client getting an eager communication and copying it directly to the object segments.
 * to the client.
 * @param clientId the libfabric client ID to know the connection to be used.
 * @param clientMessage the request from the client to get the required informations.
 * @param segments The list of object segments to be sent.
**/
void Server::objEagerExtractFromMessage(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//get base pointer
	char * data = (char*)(clientMessage + 1);

	//copy data
	size_t cur = 0;
	size_t dataSize = clientMessage->data.objReadWrite.size;
	size_t baseOffset = clientMessage->data.objReadWrite.offset;
	for (auto segment : segments) {
		//compute copy size to stay in data limits
		size_t copySize = segment.size;
		size_t offset = 0;
		if (cur + copySize > dataSize) {
			copySize = dataSize - cur;
		}
		if (baseOffset > segment.offset) {
			offset = baseOffset - segment.offset;
			copySize -= offset;
		}

		//copy
		memcpy(segment.ptr + offset, data + cur, copySize);

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
		return LF_WAIT_LOOP_KEEP_WAITING;
	});
}

/****************************************************/
/**
 * Register the hook for OBJ_WRITE messages and apply the flush on reception, then
 * answer with an ACK message after making an eager (from the recived message) or an rdam transfer.
**/
void Server::setupObjWrite(void)
{
	//register hook
	this->connection->registerHook(IOC_LF_MSG_OBJ_WRITE, [this](LibfabricConnection * connection, int clientId, size_t id, void * buffer) {
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
		connection->repostRecive(id);

		return LF_WAIT_LOOP_KEEP_WAITING;
	});
}

/****************************************************/
/**
 * On TCP client connect we register the client to the libfabric connection
 * so it can check the client auth to let him sending messages to the server.
 * @param tcpClientId Define the TCP client ID.
 * @param key The key attached to this client to validate incoming messages.
**/
void Server::onClientConnect(uint64_t tcpClientId, uint64_t key)
{
	printf("Client connect ID=%lu, key=%lu\n", tcpClientId, key);
	connection->getClientRegistry().registerClient(tcpClientId, key);
}

/****************************************************/
/**
 * On client disconnection we need to remove it from the allowed client list in
 * the connection.
**/
void Server::onClientDisconnect(uint64_t tcpClientId)
{
	printf("Client disconnect ID=%lu\n", tcpClientId);
	connection->getClientRegistry().disconnectClient(tcpClientId);
	container->onClientDisconnect(tcpClientId);
}
