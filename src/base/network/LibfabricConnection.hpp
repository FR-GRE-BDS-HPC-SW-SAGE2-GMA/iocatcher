/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_LIBFABRIC_CONNECTION_HPP
#define IOC_LIBFABRIC_CONNECTION_HPP

/****************************************************/
//std
#include <functional>
#include <map>
#include <cassert>
//libfabric
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_endpoint.h>
//local
#include "LibfabricDomain.hpp"
#include "Serializer.hpp"
#include "Protocol.hpp"
#include "Hook.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/** Position of the server in the address list. **/
#define IOC_LF_SERVER_ID 0
/** No action. **/
#define IOC_LF_NO_WAKEUP_POST_ACTION ((LibfabricPostAction*)-1)
/** Has no buffer attached to the post action so nothing to repost in the recive queue. **/
#define IOC_LF_NO_BUFFER ((size_t)-1)

/****************************************************/
class LibfabricConnection;

/****************************************************/
/**
 * Define a buffer with its size.
**/
struct LibfabricBuffer
{
	void * buffer;
	size_t size;
};

/****************************************************/
/**
 * Define a post action when we receive a message or when a RDMA operation finishes.
 * @brief Post action on RDMA operations.
**/
class LibfabricPostAction
{
	public:
		LibfabricPostAction(void);
		virtual ~LibfabricPostAction(void);
		virtual LibfabricActionResult runPostAction(void) = 0;
		void registerBuffer(LibfabricConnection * connection, bool isRecv, size_t bufferId);
		void freeBuffer(void);
		void attachDomainBuffer(LibfabricConnection * connection, void * domainBuffer);
	protected:
		/** Keep track of the connection. **/
		LibfabricConnection * connection;
		/** If the registered buffer id correspond to a recive buffer. **/
		bool isRecv;
		/** A buffer ID to clean when then post action terminate. **/
		size_t bufferId;
		/** keep track of the buffer. **/
		void * domainBuffer;
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
 * Implement the core part of the post operation via a lambda function passed to
 * the constructor.
 * @brief Post action based on a lambda operation.
**/
class LibfabricPostActionNop : public LibfabricPostAction
{
	public:
		LibfabricPostActionNop(LibfabricActionResult result) {this->result = result;};
		virtual LibfabricActionResult runPostAction(void);
	private:
		LibfabricActionResult result;
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
		bool pollMessage(LibfabricRemoteResponse & response, LibfabricMessageType expectedMessageType);
		void setHooks(std::function<void(int)> hookOnEndpointConnect);
		void broadcastErrrorMessage(const std::string & message);
		template <class T> void sendMessage(LibfabricMessageType msgType, int destinationEpId, T & data, LibfabricPostAction * postAction);
		template <class T> void sendMessage(LibfabricMessageType msgType, int destinationEpId, T & data, std::function<LibfabricActionResult(void)> postAction);
		template <class T> void sendMessageNoPollWakeup(LibfabricMessageType msgType, int destinationEpId, T & data);
		void sendMessage(void * buffer, size_t size, int destinationEpId, std::function<LibfabricActionResult(void)> postAction);
		void sendMessageNoPollWakeup(void * buffer, size_t size, int destinationEpId);
		void rdmaRead(int destinationEpId, void * localAddr, LibfabricAddr remoteAddr, uint64_t remoteKey, size_t size, std::function<LibfabricActionResult(void)> postAction);
		void rdmaReadv(int destinationEpId, struct iovec * iov, int count, LibfabricAddr remoteAddr, uint64_t remoteKey, std::function<LibfabricActionResult(void)> postAction);
		void rdmaWrite(int destinationEpId, void * localAddr, LibfabricAddr remoteAddr, uint64_t remoteKey, size_t size, std::function<LibfabricActionResult(void)> postAction);
		void rdmaWritev(int destinationEpId, struct iovec * iov, int count, LibfabricAddr remoteAddr, uint64_t remoteKey, std::function<LibfabricActionResult(void)> postAction);
		void repostReceive(size_t id);
		void repostReceive(const LibfabricClientRequest & request);
		void registerHook(int messageType, Hook * hook);
		void registerHook(int messageType, HookLambdaDef function);
		void unregisterHook(int messageType);
		int getClientId(void) { return clientId;};
		LibfabricDomain & getDomain(void) {return *lfDomain;};
		void setUsed(bool used) {this->used = used;};
		bool getUsed(void) {return this->used;}
		void setTcpClientInfos(uint64_t tcpClientId, uint64_t tcpClientKey);
		void fillProtocolHeader(LibfabricMessageHeader & header, uint64_t type);
		ClientRegistry & getClientRegistry(void);
		void setCheckClientAuth(bool value);
		void setOnBadAuth(std::function<LibfabricActionResult(void)> hookOnBadAuth);
		void sendResponse(LibfabricMessageType msgType, uint64_t lfClientId, int32_t status, bool unblock = false);
		void sendResponse(LibfabricMessageType msgType, uint64_t lfClientId, int32_t status, const char * data, size_t size, bool unblock = false);
		void sendResponse(LibfabricMessageType msgType, uint64_t lfClientId, int32_t status, const LibfabricBuffer * buffers, size_t cntBuffers, bool unblock = false);
	private:
		void sendRawMessage(void * buffer, size_t size, int destinationEpId, LibfabricPostAction * postAction);
		bool pollRx(void);
		bool pollTx(void);
		int pollForCompletion(struct fid_cq * cq, struct fi_cq_msg_entry* entry, bool passivePolling, bool acceptCache = true);
		LibfabricActionResult onRecv(size_t id);
		bool onRecvMessage(LibfabricRemoteResponse & response, size_t id);
		void onSent(void * buffer);
		void onConnInit(LibfabricMessage * message);
		bool checkAuth(LibfabricMessage * message, uint64_t clientId, int id);
		void pollAllCqInCache(void);
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
		uint64_t nextEndpointId;
		/** Hook to be called when a client connect. **/
		std::function<void(int)> hookOnEndpointConnect;
		/** Hook to be called when we recive a message with wrong authentication. **/
		std::function<LibfabricActionResult(void)> hookOnBadAuth;
		/** Keep track of the client ID if used for client side. **/
		uint64_t clientId;
		/** 
		 * Keep tack of the lambda functions to be called when reciveing
		 * the associated message ID.
		**/
		std::map<uint64_t, Hook *> hooks;
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
		/** To be used when broacasting a crash message. **/
		bool disableReceive;
		/** Buffer to store batch readed completion queue entries **/
		std::list<fi_cq_msg_entry> cqEntries;
};

/****************************************************/
/**
 * Serialize the given structure and send it as a message.
 * @param msgType Define the type of message.
 * @param destinationEpId Define the ID of the remote entity to target.
 * @param data Define the value to serialize and send.
 * @param portAction Provide a lambda function to be called when the message has been sent.
**/
template <class T>
void LibfabricConnection::sendMessage(LibfabricMessageType msgType, int destinationEpId, T & data, std::function<LibfabricActionResult(void)> postAction)
{
	this->sendMessage(msgType, destinationEpId, data, new LibfabricPostActionFunction(postAction));
}

/****************************************************/
/**
 * Serialize the given structure and send it as a message.
 * This variant of the sendMessage() function do not wake up the poll()
 * function when the message has been sent.
 * It aims to be used in client code implementation where we send a request
 * then poll for a specific message with the pollMessage() function which
 * needs to wake up only when it recived the response from the server.
 * @param msgType Define the type of message.
 * @param destinationEpId Define the ID of the remote entity to target.
 * @param data Define the value to serialize and send.
**/
template <class T> 
void LibfabricConnection::sendMessageNoPollWakeup(LibfabricMessageType msgType, int destinationEpId, T & data)
{
	this->sendMessage(msgType, destinationEpId, data, new LibfabricPostActionNop(LF_WAIT_LOOP_KEEP_WAITING));
}

/****************************************************/
/**
 * Serialize the given structure and send it as a message.
 * @param msgType Define the type of message.
 * @param destinationEpId Define the ID of the remote entity to target.
 * @param data Define the value to serialize and send.
 * @param portAction Provide a lambda function to be called when the message has been sent.
**/
template <class T>
void LibfabricConnection::sendMessage(LibfabricMessageType msgType, int destinationEpId, T & data, LibfabricPostAction * postAction)
{
	//check
	assert(postAction != NULL);
	assert(postAction != IOC_LF_NO_WAKEUP_POST_ACTION);

	//get a buffer
	void * buffer = this->lfDomain->getMsgBuffer();
	size_t bufferSize = this->lfDomain->getMsgBufferSize();

	//register it to be freed when the postaction will be deleted
	postAction->attachDomainBuffer(this, buffer);

	//build the header
	LibfabricMessageHeader header;
	this->fillProtocolHeader(header, msgType);

	//serialize
	Serializer serializer(buffer, bufferSize);
	serializer.apply("header", header);
	serializer.apply("data", data);

	//extract size
	size_t finalSize = serializer.getCursor();

	//send the message
	this->sendRawMessage(buffer, finalSize, destinationEpId, postAction);
}

}

#endif //IOC_LIBFABRIC_CONNECTION_HPP
