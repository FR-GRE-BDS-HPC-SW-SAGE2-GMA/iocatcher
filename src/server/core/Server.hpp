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
//local
#include "Config.hpp"
#include "Container.hpp"
#include "ServerStats.hpp"
#include "../../base/network/LibfabricDomain.hpp"
#include "../../base/network/LibfabricConnection.hpp"
#include "../../base/network/TcpServer.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement the RDMA server to access objects with a cache in memory or nvdimm.
**/
class Server
{
	public:
		Server(const Config * config, const std::string & port);
		~Server(void);
		void poll(void);
		void startStatsThread(void);
		void stop(void);
		void setOnClientConnect(std::function<void(int id)> handler);
		Container & getContainer(void) {return *this->container;};
	private:
		//setups
		void setupTcpServer(int port, int maxport);
		//conn tracking
		void onClientConnect(uint64_t id, uint64_t key);
		void onClientDisconnect(uint64_t id);
	private:
		/** The libfabric domain to be used for memory registration. **/
		LibfabricDomain * domain;
		/** The connection to be used to exchange messages wuth clients and make RDMA operations. **/
		LibfabricConnection * connection;
		/** The container to store objects. **/
		Container * container;
		/** keep track of IO statistics. **/
		ServerStats stats;
		/** Pointer to the config object. **/
		const Config * config;
		/** Thread printing the stats in the termianl every seconds. **/
		std::thread statThread;
		/** Thread handling the TCP connection to accept incomming clients and be notified of client disconnection. **/
		std::thread tcpServerThread;
		/** True while we need to continue to active poll. **/
		volatile bool pollRunning;
		/** To know if the stat thread need to continue to run or need to exit. **/
		volatile bool statsRunning;
		/** Pointer to the TCP server. **/
		TcpServer * tcpServer;
};

}

#endif //IOC_SERVER_ACTIONS_HPP
