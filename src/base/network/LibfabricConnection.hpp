/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_LIBFABRIC_CONNECTION_HPP
#define IOC_LIBFABRIC_CONNECTION_HPP

/****************************************************/
//std
#include <functional>
#include <map>
//libfabric
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_endpoint.h>
//local
#include "LibfabricDomain.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
#define IOC_LF_MAX_ADDR_LEN 32
#define IOC_LF_SERVER_ID 0

/****************************************************/
enum LibfabricMessageType
{
	IOC_LF_MSG_CONNECT_INIT,
	IOC_LF_MSG_ASSIGN_ID
};

/****************************************************/
struct LibfabricMessage
{
	LibfabricMessageType type;
	union {
		/** Client to address to be send to the server on MSG_CONN_INIT **/
		char addr[IOC_LF_MAX_ADDR_LEN];
		/** Client ID so the server know to who to respond. **/
		int id;
	} data;
};

/****************************************************/
class LibfabricConnection
{
	public:
		LibfabricConnection(LibfabricDomain * lfDomain, bool wait);
		~LibfabricConnection(void);
		void postRecives(size_t size, int count);
		void joinServer(void);
		void poll(void);
		void setHooks(std::function<void(int)> hookOnEndpointConnect);
		void sendMessage(void * buffer, size_t size, int destinationEpId);
	private:
		bool pollRx(void);
		bool pollTx(void);
		int pollForCompletion(struct fid_cq * cq, void ** context, bool wait);
		void onRecv(size_t id);
		void onConnInit(LibfabricMessage * message);
		void repostRecive(size_t id);
	private:
		LibfabricDomain * lfDomain;
		/** Transmission completion queue. **/
		fid_cq *tx_cq;
		//** Revive completion queue. **/
		fid_cq *rx_cq;
		/** Address vector. **/
		fid_av *av;
		/** Endpoint **/
		fid_ep *ep;
		/** If use wait **/
		bool wait;
		/** buffers **/
		char ** recvBuffers;
		int recvBuffersCount;
		size_t recvBuffersSize;
		std::map<int, fi_addr_t> remoteLiAddr;
		int nextEndpointId;
		std::function<void(int)> hookOnEndpointConnect;
};

}

#endif //IOC_LIBFABRIC_CONNECTION_HPP
