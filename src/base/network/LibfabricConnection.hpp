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
#include "Protocol.hpp"
#include "Hook.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/** Position of the server in the address list. **/
#define IOC_LF_SERVER_ID 0
#define IOC_LF_NO_WAKEUP_POST_ACTION ((void*)-1)

/****************************************************/
class LibfabricConnection;

/****************************************************/
/**
 * Define a post action when we recive a message or when an RDMA operation finish.
 * @brief Post action on RDMA operations.
**/
class LibfabricPostAction
{
	public:
		LibfabricPostAction(void) {this->bufferId = -1; this->connection = NULL;};
		virtual ~LibfabricPostAction(void) {this->freeBuffer();};
		virtual LibfabricActionResult runPostAction(void) = 0;
		void registerBuffer(LibfabricConnection * connection, bool isRecv, size_t bufferId);
		void freeBuffer(void);
	protected:
		/** Keep track of the connection. **/
		LibfabricConnection * connection;
		/** If the registered buffer id correspond to a recive buffer. **/
		bool isRecv;
		/** A buffer ID to clean when then post action terminate. **/
		size_t bufferId;
};

/****************************************************/
/**
 * Implement the core part of the post operation via a lambda function passed to
 * the constructor.
 * @brief Post action based on a lambda operation.
**/
class LibfabricPostActionFunction : public LibfabricPostAction
{
	public:
		LibfabricPostActionFunction(std::function<LibfabricActionResult(void)> function) {this->function = function;};
		virtual LibfabricActionResult runPostAction(void);
	private:
		/** Keep track of the lambda operation. **/
		std::function<LibfabricActionResult(void)> function;
};

/****************************************************/
/**
 * Structure used to return the message info from pollMessage.
**/
struct LibfabricClientMessage
{
	/** ID of the client which sends the message. **/
	int lfClientId;
	/** Message buffer to be returned to the connection after using the data.**/
	size_t msgBufferId;
	/** Pointer to the client message. **/
	LibfabricMessage * message;
};


/****************************************************/
/**
 * Handling wrapper to managment a libfabric connection. It provides the 
 * necessary semantic to recive messages, send messages and make RDMA operation 
 * and be notified when they finish. It handle the client side and the server
 * side to get the same class to handle all communications.
 * @brief Wrapper to handle a libfabric connection.
**/
class LibfabricConnection
{
	public:
		LibfabricConnection(LibfabricDomain * lfDomain, bool passivePolling);
		~LibfabricConnection(void);
		void postRecives(size_t size, int count);
		void joinServer(void);
		void poll(bool waitMsg);
		bool pollMessage(LibfabricClientMessage & clientMessage, LibfabricMessageType expectedMessageType);
		void setHooks(std::function<void(int)> hookOnEndpointConnect);
		void sendMessage(void * buffer, size_t size, int destinationEpId, std::function<LibfabricActionResult(void)> postAction);
		void sendMessageNoPollWakeup(void * buffer, size_t size, int destinationEpId);
		void rdmaRead(int destinationEpId, void * localAddr, void * remoteAddr, uint64_t remoteKey, size_t size, std::function<LibfabricActionResult(void)> postAction);
		void rdmaReadv(int destinationEpId, struct iovec * iov, int count, void * remoteAddr, uint64_t remoteKey, std::function<LibfabricActionResult(void)> postAction);
		void rdmaWrite(int destinationEpId, void * localAddr, void * remoteAddr, uint64_t remoteKey, size_t size, std::function<LibfabricActionResult(void)> postAction);
		void rdmaWritev(int destinationEpId, struct iovec * iov, int count, void * remoteAddr, uint64_t remoteKey, std::function<LibfabricActionResult(void)> postAction);
		void repostRecive(size_t id);
		void repostRecive(const LibfabricClientMessage & clientMessage);
		void registerHook(int messageType, Hook * hook);
		void registerHook(int messageType, std::function<LibfabricActionResult(LibfabricConnection *, int, size_t, void*)> function);
		void unregisterHook(int messageType);
		int getClientId(void) { return clientId;};
		LibfabricDomain & getDomain(void) {return *lfDomain;};
		void setUsed(bool used) {this->used = used;};
		bool getUsed(void) {return this->used;}
		void setTcpClientInfos(uint64_t tcpClientId, uint64_t tcpClientKey);
		void fillProtocolHeader(LibfabricMessageHeader & header, int type);
		ClientRegistry & getClientRegistry(void);
		void setCheckClientAuth(bool value);
		void setOnBadAuth(std::function<LibfabricActionResult(void)> hookOnBadAuth);
	private:
		bool pollRx(void);
		bool pollTx(void);
		int pollForCompletion(struct fid_cq * cq, struct fi_cq_msg_entry* entry, bool passivePolling);
		LibfabricActionResult onRecv(size_t id);
		bool onRecvMessage(LibfabricClientMessage & clientMessage, size_t id);
		void onSent(void * buffer);
		void onConnInit(LibfabricMessage * message);
		bool checkAuth(LibfabricMessage * message, int clientId, int id);
	private:
		/** Pointer to the libfabric domain to be used to establish the connection. **/
		LibfabricDomain * lfDomain;
		/** Completion queue. **/
		fid_cq *cq;
		/** Address vector. **/
		fid_av *av;
		/** Endpoint **/
		fid_ep *ep;
		/** If use wait **/
		bool passivePolling;
		/** Recive buffers being posted to libfabric. **/
		char ** recvBuffers;
		/** Number of recive buffer. **/
		size_t recvBuffersCount;
		/** Size of each recive buffer. **/
		size_t recvBuffersSize;
		/** Map of remote addresses to be used to send messages or rdma operation.**/
		std::map<int, fi_addr_t> remoteLiAddr;
		/** Keep track of the next ID to assign to the endpoints. **/
		int nextEndpointId;
		/** Hook to be called when a client connect. **/
		std::function<void(int)> hookOnEndpointConnect;
		/** Hook to be called when we recive a message with wrong authentication. **/
		std::function<LibfabricActionResult(void)> hookOnBadAuth;
		/** Keep track of the client ID if used for client side. **/
		int clientId;
		/** 
		 * Keep tack of the lambda functions to be called when reciveing
		 * the associated message ID.
		**/
		std::map<size_t, Hook *> hooks;
		/**
		 * To know if the connection is in use of not to handle multiple client
		 * connections for multi-threaded applications.
		**/
		bool used;
		/** 
		 * Keep track of the client ID assigned by the TCP connection. 
		 * It will be send for client authentication on every message to the server.
		**/
		uint64_t tcpClientId;
		/** 
		 * Keep track of the authentication key assigned by the TCP connection.
		 * It will be send for client authentication on every message to the server.
		**/
		uint64_t tcpClientKey;
		/**
		 * Keep track of the client list (used on the server side only) **/
		ClientRegistry clientRegistry;
		/** Enable of disable client authentication checking. **/
		bool checkClientAuth;
};

}

#endif //IOC_LIBFABRIC_CONNECTION_HPP
