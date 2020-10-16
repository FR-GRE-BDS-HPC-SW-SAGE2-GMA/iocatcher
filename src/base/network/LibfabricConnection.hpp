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
	IOC_LF_MSG_ASSIGN_ID,
	IOC_LF_MSG_MAX,
};

/****************************************************/
struct LibfabricMessageHeader
{
	int type;
	int clientId;
};

/****************************************************/
struct LibfabricMessage
{
	LibfabricMessageHeader header;
	union {
		/** Client to address to be send to the server on MSG_CONN_INIT **/
		char addr[IOC_LF_MAX_ADDR_LEN];
		/** Client ID so the server know to who to respond. **/
	} data;
};

/****************************************************/
class LibfabricConnection;

/****************************************************/
class LibfabricPostAction
{
	public:
		LibfabricPostAction(void) {this->bufferId = -1; this->connection = NULL;};
		virtual ~LibfabricPostAction(void) {};
		virtual bool runPostAction(void) = 0;
		void registerBuffer(LibfabricConnection * connection, bool isRecv, size_t bufferId);
		void freeBuffer(void);
	protected:
		LibfabricConnection * connection;
		bool isRecv;
		size_t bufferId;
};

/****************************************************/
class LibfabricPostActionFunction : public LibfabricPostAction
{
	public:
		LibfabricPostActionFunction(std::function<bool(LibfabricPostAction*)> function) {this->function = function;};
		virtual bool runPostAction(void);
	private:
		std::function<bool(LibfabricPostAction*)> function;
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
		void sendMessage(void * buffer, size_t size, int destinationEpId, LibfabricPostAction * postAction);
		void repostRecive(size_t id);
		void registerHook(int messageType, std::function<bool(int, size_t, void*)> function);
		void unregisterHook(int messageType);
		int getClientId(void) { return clientId;};
	private:
		bool pollRx(void);
		bool pollTx(void);
		int pollForCompletion(struct fid_cq * cq, struct fi_cq_msg_entry* entry, bool wait);
		bool onRecv(size_t id);
		void onSent(void * buffer);
		void onConnInit(LibfabricMessage * message);
	private:
		LibfabricDomain * lfDomain;
		/** Completion queue. **/
		fid_cq *cq;
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
		int clientId;
		std::map<size_t, std::function<bool(int, size_t, void*)>> hooks;
};

}

#endif //IOC_LIBFABRIC_CONNECTION_HPP
