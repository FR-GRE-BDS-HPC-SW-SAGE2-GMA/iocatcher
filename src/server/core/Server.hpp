/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_SERVER_ACTIONS_HPP
#define IOC_SERVER_ACTIONS_HPP

/****************************************************/
//std
#include <thread>
//linux
#include <sys/uio.h>
//local
#include "Container.hpp"
#include "../../base/network/LibfabricDomain.hpp"
#include "../../base/network/LibfabricConnection.hpp"
#include "../../base/network/TcpServer.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
struct ServerStats
{
	ServerStats(void);
	size_t readSize;
	size_t writeSize;
};

/****************************************************/
class Server
{
	public:
		Server(const std::string & address, const std::string & port, bool activePooling, bool consistencyCheck);
		~Server(void);
		void poll(void);
		void startStatsThread(void);
		void stop(void);
		void setOnClientConnect(std::function<void(int id)> handler);
		Container & getContainer(void) {return *this->container;};
	private:
		//setups
		void setupTcpServer(int port, int maxport);
		void setupPingPong(void);
		void setupObjFlush(void);
		void setupObjRangeRegister(void);
		void setupObjUnregisterRange(void);
		void setupObjCreate(void);
		void setupObjRead(void);
		void setupObjWrite(void);
		//internal
		iovec * buildIovec(ObjectSegmentList & segments, size_t offset, size_t size);
		void objRdmaPushToClient(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
		void objEagerPushToClient(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
		void objRdmaFetchFromClient(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
		void objEagerExtractFromMessage(int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
		//conn tracking
		void onClientConnect(uint64_t id, uint64_t key);
		void onClientDisconnect(uint64_t id);
	private:
		LibfabricDomain * domain;
		LibfabricConnection * connection;
		Container * container;
		ServerStats stats;
		bool consistencyCheck;
		std::thread statThread;
		std::thread tcpServerThread;
		volatile bool pollRunning;
		volatile bool statsRunning;
		TcpServer * tcpServer;
};

/****************************************************/
//TODO work to clean this
extern bool gblConsistencyCheck;
extern size_t gblReadSize;
extern size_t gblWriteSize;

/****************************************************/
/*
void setupPingPong(LibfabricConnection & connection);
void setupObjFlush(LibfabricConnection & connection, Container & container);
void setupObjRangeRegister(LibfabricConnection & connection, Container & container);
void setupObjUnregisterRange(LibfabricConnection & connection, Container & container);
void setupObjCreate(LibfabricConnection & connection, Container & container);
iovec * buildIovec(ObjectSegmentList & segments, size_t offset, size_t size);
void objRdmaPushToClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
void objEagerPushToClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
void setupObjRead(LibfabricConnection & connection, Container & container);
void objRdmaFetchFromClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
void objEagerExtractFromMessage(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
void setupObjWrite(LibfabricConnection & connection, Container & container);
*/

}

#endif //IOC_SERVER_ACTIONS_HPP
