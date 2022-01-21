/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <unistd.h>
#include <cstring>
#include <random>
#include <cassert>
#include "Server.hpp"
#include "Consts.hpp"
#include "StorageBackend.hpp"
#include "../hooks/HookPingPong.hpp"
#include "../hooks/HookFlush.hpp"
#include "../hooks/HookRangeRegister.hpp"
#include "../hooks/HookRangeUnregister.hpp"
#include "../hooks/HookObjectCreate.hpp"
#include "../hooks/HookObjectRead.hpp"
#include "../hooks/HookObjectWrite.hpp"
#include "../hooks/HookObjectCow.hpp"
#include "base/common/Debug.hpp"
#include "../backends/MemoryBackendCache.hpp"
#include "../backends/MemoryBackendBalance.hpp"
#include "../backends/MemoryBackendNvdimm.hpp"
#include "../backends/MemoryBackendMalloc.hpp"

/****************************************************/
using namespace IOC;

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

	//setup domain
	this->domain = new LibfabricDomain(config->listenIP, port, true);
	this->domain->setMsgBufferSize(IOC_POST_RECEIVE_READ);

	//establish connections
	this->connection = new LibfabricConnection(this->domain, !config->activePolling);
	assert(IOC_POST_RECEIVE_WRITE < 1024*1024);
	this->connection->postReceives(1024*1024, 128);
	if (config->clientAuth)
		this->connection->setCheckClientAuth(true);

	//setup tcp server
	int tcpPort = atoi(port.c_str()) + 1;
	printf("Server on port %s/%d\n", port.c_str(), tcpPort);
	this->setupTcpServer(tcpPort, tcpPort);

	//spawn storage backend
	this->storageBackend = NULL;
	this->memoryBackend = new MemoryBackendCache(new MemoryBackendMalloc(domain));

	//create container
	this->container = new Container(storageBackend, memoryBackend, 8*1024*1024);

	//register hooks
	this->connection->registerHook(IOC_LF_MSG_PING, new HookPingPong(this->domain));
	this->connection->registerHook(IOC_LF_MSG_OBJ_FLUSH, new HookFlush(this->container));
	this->connection->registerHook(IOC_LF_MSG_OBJ_RANGE_REGISTER, new HookRangeRegister(this->config, this->container));
	this->connection->registerHook(IOC_LF_MSG_OBJ_RANGE_UNREGISTER, new HookRangeUnregister(this->config, this->container));
	this->connection->registerHook(IOC_LF_MSG_OBJ_CREATE, new HookObjectCreate(this->container));
	this->connection->registerHook(IOC_LF_MSG_OBJ_READ, new HookObjectRead(this->container, &this->stats));
	this->connection->registerHook(IOC_LF_MSG_OBJ_WRITE, new HookObjectWrite(this->container, &this->stats));
	this->connection->registerHook(IOC_LF_MSG_OBJ_COW, new HookObjectCow(this->container));

	//set error dispatch
	if (config->broadcastErrorToClients) {
		DAQ::Debug::setBeforeAbortHandler([this](const std::string & message){
			//keep track of first send to avoid looping if a second error
			//append in broadcastErrrorMessage().
			static bool first = true;

			//get current
			bool ok = first;
			first = false;

			//only first time
			if (ok)
				this->connection->broadcastErrrorMessage(message);
		});
	}
}

/****************************************************/
/**
 * Clean the sub objects.
**/
Server::~Server(void)
{
	this->stop();
	delete this->container;
	delete this->memoryBackend;
	delete this->connection;
	delete this->domain;
	delete this->tcpServer;
}

/****************************************************/
/**
 * Attach a storage backend to the server.
 * @param storageBackend Pointer to the backend to be used.
**/
void Server::setStorageBackend(StorageBackend * storageBackend)
{
	this->storageBackend = storageBackend;
	this->container->setStorageBackend(storageBackend);
}

/****************************************************/
/**
 * Attach a memory backend to the server.
 * @param memoryBackend Pointer to the backend to be used.
**/
void Server::setMemoryBackend(MemoryBackend * memoryBackend)
{
	//keep old to delete
	MemoryBackend * old = this->memoryBackend;

	//replace in chiles
	this->memoryBackend = memoryBackend;
	this->container->setMemoryBackend(memoryBackend);

	//delete old
	delete old;
}

/****************************************************/
/**
 * Setup the TCP server to receive new clients. It starts a new thread
 * to handle the connection. It also provides the handler to assign
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
		this->tcpServerThread.join();
		delete this->tcpServer;
		this->tcpServer = NULL;
	}
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
	IOC_DEBUG_ARG("client:tcp", "Client connect, assign tcpId=%1, key=%2")
		.arg(tcpClientId)
		.arg(key)
		.end();
	connection->getClientRegistry().registerClient(tcpClientId, key);
}

/****************************************************/
/**
 * On client disconnection we need to remove it from the allowed client list in
 * the connection.
**/
void Server::onClientDisconnect(uint64_t tcpClientId)
{
	IOC_DEBUG_ARG("client:tcp", "Client disconnect tcpId=%1").arg(tcpClientId).end();
	connection->getClientRegistry().disconnectClient(tcpClientId);
	container->onClientDisconnect(tcpClientId);
}

/****************************************************/
void Server::setNvdimm(const std::vector<std::string> & nvdimmPaths)
{
	//set root round robin backend
	MemoryBackendBalance * backend = new MemoryBackendBalance();

	//loop to add all childs
	for (auto & it : nvdimmPaths) {
		//allocate low level backend
		MemoryBackendNvdimm * lowLevelBackend = new MemoryBackendNvdimm(this->domain, it);

		//setup cache
		MemoryBackendCache * cache = new MemoryBackendCache(lowLevelBackend);

		//register to round robin
		backend->registerBackend(cache);
	}

	//setup
	this->setMemoryBackend(backend);
}
