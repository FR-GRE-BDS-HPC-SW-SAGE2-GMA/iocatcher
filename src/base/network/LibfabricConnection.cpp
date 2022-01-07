/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cassert>
#include <cstring>
//libfabric
#include "rdma/fi_cm.h"
#include "rdma/fi_rma.h"
//linux
#include <sys/uio.h>
//local
#include "LibfabricConnection.hpp"
#include "HookLambdaFunction.hpp"
#include "../common/Debug.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * On deletion of the object we free the attached recieve buffer to return it to
 * the connection.
**/
void LibfabricPostAction::freeBuffer(void)
{
	if (connection != NULL)
		if (isRecv)
			connection->repostReceive(bufferId);
}

/****************************************************/
/**
 * On post action we call the lambda function.
**/
LibfabricActionResult LibfabricPostActionFunction::runPostAction(void)
{
	return this->function();
}

/****************************************************/
/**
 * Constructor used to establigh a new connection handler.
 * @param lfDomain Libfabric domain to be used. This should not be NULL.
 * @param passivePolling Use a passive polling if true and an active polling it false.
 **/
LibfabricConnection::LibfabricConnection(LibfabricDomain * lfDomain, bool passivePolling)
{
	//check
	assert(lfDomain != NULL);

	//set
	this->used = false;
	this->clientId = -1;
	this->lfDomain = lfDomain;
	this->passivePolling = passivePolling;
	this->recvBuffersCount = 0;
	this->recvBuffers = NULL;
	this->nextEndpointId = 0;
	this->recvBuffersSize = 0;
	this->tcpClientId = 0;
	this->tcpClientKey = 0;
	this->checkClientAuth = false;
	this->disableReceive = false;

	//debug
	IOC_DEBUG("libfabric:conn", "Create new connection");

	//extract
	fi_info *fi = lfDomain->getFiInfo();
	fid_domain *domain = lfDomain->getDomain();

	//vars
	int err;

	//to init completion queues
	fi_cq_attr cq_attr;
	fi_av_attr av_attr;
	memset(&cq_attr, 0, sizeof(cq_attr));
	memset(&av_attr, 0, sizeof(av_attr));
	
	//setup attr
	cq_attr.format = FI_CQ_FORMAT_MSG;
	cq_attr.size = fi->tx_attr->size;
	if (passivePolling)
		cq_attr.wait_obj = FI_WAIT_UNSPEC;
	else
		cq_attr.wait_obj = FI_WAIT_NONE;
	//printf("CQ_SIZE: %ld\n", cq_attr.size);

	//setup cq
	err = fi_cq_open(domain, &cq_attr, &this->cq, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_cq_open",err);

	//addres vector
	//av_attr.type = FI_AV_MAP;
	err = fi_av_open(domain, &av_attr, &this->av, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_av_open",err);

	//create endpoint
	err = fi_endpoint(domain, fi, &this->ep, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_endpoint",err);

	//bind completion queue
	err = fi_ep_bind(this->ep, (struct fid *)this->cq, FI_TRANSMIT|FI_RECV);
	LIBFABRIC_CHECK_STATUS("fi_ep_bind",err);

	//bind
	err = fi_ep_bind(this->ep, &this->av->fid, 0);
	LIBFABRIC_CHECK_STATUS("fi_ep_bind", err);

	//enable endpoint
	err = fi_enable(this->ep);
	LIBFABRIC_CHECK_STATUS("fi_enable", err);
}

/****************************************************/
/**
 * Destructor of the connection, it frees all ressources attached to the connection.
**/
LibfabricConnection::~LibfabricConnection(void)
{
	//close
	fi_close((fid_t)ep);
	fi_close((fid_t)av);
	fi_close((fid_t)cq);

	//destroy buffers
	if (this->recvBuffers != NULL) {
		for (size_t i = 0 ; i < recvBuffersCount ; i++) {
			delete [] this->recvBuffers[i];
		}
		delete [] this->recvBuffers;
	}

	//destroy hooks
	for (auto & it : this->hooks)
		delete it.second;
}

/****************************************************/
/**
 * Allocate recive buffer and pose them to libfabric.
 * @param size Size of the recive buffer.
 * @param count Number of recive buffer to allocate and post.
**/
void LibfabricConnection::postRecives(size_t size, int count)
{
	//setup
	this->recvBuffersCount = count;
	this->recvBuffersSize = size;

	//allocate array
	this->recvBuffers = new char * [count];

	//allocate buffers
	for (int i = 0 ; i < count ; i++) {
		//alloc
		this->recvBuffers[i] = new char[size];

		//register for reception
		//fid_mr * mr;
		//int err = fi_mr_reg(this->lfDomain->getDomain(), this->recvBuffers[i], size, FI_RECV, 0, 0, 0, &mr, NULL);
		//LIBFABRIC_CHECK_STATUS("fi_mr_reg", err);

		//post
		this->repostReceive(i);
	}
}

/****************************************************/
/**
 * Republish a recive buffer to libfabric by identifying it by its ID.
 * @param id ID of the buffer to repost.
**/
void LibfabricConnection::repostReceive(size_t id)
{
	//check
	assumeArg(id <recvBuffersCount, "Invalid recive buffer ID: %1").arg(id).end();

	//debug
	IOC_DEBUG_ARG("libfabric:conn", "Repost recive buffer %1").arg(id).end();

	//post
	int err = fi_recv(this->ep, this->recvBuffers[id], recvBuffersSize, 0, 0, (void*)id);
	LIBFABRIC_CHECK_STATUS("fi_recv", err);
}

/****************************************************/
/**
 * Republish a recive buffer to libfabric by identifying it by its ID.
 * @param request Reference to the request containing the receive buffer ID to be reposted.
**/
void LibfabricConnection::repostReceive(const LibfabricClientRequest & request)
{
	this->repostReceive(request.msgBufferId);
}

/****************************************************/
/**
 * When being a client, this function is used to join the libfabric domain server.
**/
void LibfabricConnection::joinServer(void)
{
	//vars
	int err;
	size_t addrlen = IOC_LF_MAX_ADDR_LEN;

	//debug
	IOC_DEBUG("libfabric:conn", "Join server");

	//insert server address in address vector
	err = fi_av_insert(this->av, this->lfDomain->getFiInfo()->dest_addr, 1, &this->remoteLiAddr[IOC_LF_SERVER_ID], 0, NULL);
	if (err != 1)
		LIBFABRIC_CHECK_STATUS("fi_av_insert", -1);

	//new message
	LibfabricMessage * msg = new LibfabricMessage;
	memset(msg, 0, sizeof(*msg));
	msg->header.msgType = IOC_LF_MSG_CONNECT_INIT;
	err = fi_getname(&this->ep->fid, msg->data.firstClientMessage.addr, &addrlen);
	LIBFABRIC_CHECK_STATUS("fi_getname", err);
	assert(addrlen <= IOC_LF_MAX_ADDR_LEN);

	//register hook
	this->registerHook(IOC_LF_MSG_ASSIGN_ID, [this](LibfabricConnection* connection, LibfabricClientRequest & request) {
		//assign id
		//printf("get clientID %d\n", clientId);
		this->clientId = request.lfClientId;
		connection->repostReceive(request.msgBufferId);

		//check protocol version
		assumeArg(request.message->data.firstHandshakeResponse.protocolVersion == IOC_LF_PROTOCOL_VERSION,
			"Invalid rdma protocol version from server, expected %1, got %2")
				.arg(IOC_LF_PROTOCOL_VERSION)
				.arg(request.message->data.firstHandshakeResponse.protocolVersion)
				.end();

		//return back
		return LF_WAIT_LOOP_UNBLOCK;
	});

	//send
	this->sendMessage(msg, sizeof(LibfabricMessage), IOC_LF_SERVER_ID, [msg](){
		delete msg;
		return LF_WAIT_LOOP_KEEP_WAITING;
	});

	//wait send
	this->poll(true);

	//debug
	IOC_DEBUG("libfabric:conn", "Server joined");
}

/****************************************************/
/**
 * Function to be used in case of crash to send an error
 * message to all clients before exit. It disable handling
 * of all recived messages before sending the message.
 * @param message The message to be sent.
**/
void LibfabricConnection::broadcastErrrorMessage(const std::string & message)
{
	//debug
	IOC_DEBUG_ARG("libfabric:conn", "Broadcast error message to all clients : %1").arg(message).end();

	//allocate buffer
	const size_t size = sizeof(LibfabricMessageHeader) + message.size() + 1;
	void * buffer = malloc(size);

	//disable reception
	this->disableReceive = true;

	//build message to send
	LibfabricMessage * lfMessage = static_cast<LibfabricMessage*>(buffer);
	this->fillProtocolHeader(lfMessage->header, IOC_LF_MSG_FATAL_ERROR);

	//push message
	memcpy(&lfMessage->data, message.c_str(), message.size() + 1);

	//send to all
	size_t cnt = this->remoteLiAddr.size();
	for (auto & it : this->remoteLiAddr) {
		this->sendMessage(buffer, size, it.first, [](){
			return LF_WAIT_LOOP_UNBLOCK;
		});
	}

	//wait all
	while (cnt > 0){
		this->poll(true);
		cnt--;
	}

	//free mem
	free(buffer);
}

/****************************************************/
/**
 * Send a message to the given destination ID.
 * @param buffer Buffer to be sent.
 * @param size Size of the given buffer.
 * @param destrinationEpId ID of the destination.
 * @param postAction A lambda function without parameters to be called when the message has been sent.
 * It returns a LibfabricActionResult which tell to the poll() function if it need to continue polling
 * or if it needs to return.
**/
void LibfabricConnection::sendMessage(void * buffer, size_t size, int destinationEpId, std::function<LibfabricActionResult(void)> postAction)
{
	//vars
	int err;

	//checks
	assert(buffer != NULL);
	assert(size <= recvBuffersSize);

	//debug
	IOC_DEBUG_ARG("libfabric:msg", "Send message: dest=%1, buffer=%2").arg(destinationEpId).arg(buffer).end();

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	//send
	LibfabricPostActionFunction * handler = new LibfabricPostActionFunction(postAction);
	do {
		err = fi_send(this->ep, buffer, size, NULL, it->second, handler);
		if (err == -FI_EAGAIN)
			this->pollAllCqInCache();
	} while(err == -FI_EAGAIN);
	LIBFABRIC_CHECK_STATUS("fi_send", err);
}

/****************************************************/
/**
 * Send a message to the given destination ID. This variant of sendMessage() by default do
 * not unlock the poll() and pollMessage() functions when the message has been sent.
 * @param buffer Buffer to be sent.
 * @param size Size of the given buffer.
 * @param destrinationEpId ID of the destination.
**/
void LibfabricConnection::sendMessageNoPollWakeup(void * buffer, size_t size, int destinationEpId)
{
	//vars
	int err;

	//checks
	assert(buffer != NULL);
	assert(size <= recvBuffersSize);

	//debug
	IOC_DEBUG_ARG("libfabric:msg", "Send message without poll wakeup: dest=%1, buffer=%2").arg(destinationEpId).arg(buffer).end();

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	//send
	do {
		err = fi_send(this->ep, buffer, size, NULL, it->second, IOC_LF_NO_WAKEUP_POST_ACTION);
		if (err == -FI_EAGAIN)
			this->pollAllCqInCache();
	} while(err == -FI_EAGAIN);
	LIBFABRIC_CHECK_STATUS("fi_send", err);
}

/****************************************************/
/**
 * Start a rdma read operation to fetch data from the remote server.
 * @param destrinationEpId ID of the destination.
 * @param localAddr The local address where to place the data.
 * @param remoteAddr The remote address to read from.
 * @param remoteKey The remote key to be allowed to read the remote segment.
 * @param size the size to read.
 * @param postAction A lambda function without parameters to be called when the message has been sent.
 * It returns a LibfabricActionResult which tell to the poll() function if it need to continue polling
 * or if it needs to return.
**/
void LibfabricConnection::rdmaRead(int destinationEpId, void * localAddr, LibfabricAddr remoteAddr, uint64_t remoteKey, size_t size, std::function<LibfabricActionResult(void)> postAction)
{
	//check
	assert(localAddr != NULL);

	//debug
	IOC_DEBUG_ARG("libfabric:rdma", "Start RDMA read: dest=%1, localAddr=%2, remoteAddr=%3, size=%4")
		.arg(destinationEpId)
		.arg(localAddr)
		.arg(remoteAddr)
		.arg(size)
		.end();

	//get
	fid_mr * mr = lfDomain->getFidMR(localAddr,size);
	void * mrDesc = fi_mr_desc(mr);
	assert(mr != NULL);

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	//do action and retry while we got FI_EAGAIN error
	int ret = 0;
	do {
		ret = fi_read(ep, localAddr, size, mrDesc, it->second, (uint64_t)remoteAddr, remoteKey, new LibfabricPostActionFunction(postAction));
		if (ret == -FI_EAGAIN)
			this->pollAllCqInCache();
	} while(ret == -FI_EAGAIN);
	LIBFABRIC_CHECK_STATUS("fi_read", ret);
}

/****************************************************/
/**
 * Start a rdma read operation to fetch data from the remote server.
 * This implementation consider the case where the local memory is spreaded
 * over multiple segments described by the IOv array.
 * @param destrinationEpId ID of the destination.
 * @param iov The local io vector describing the list of segments where to place the data.
 * @param count The number of local segments in the iov.
 * @param remoteAddr The remote address to read from.
 * @param remoteKey The remote key to be allowed to read the remote segment.
 * @param size the size to read.
 * @param postAction A lambda function without parameters to be called when the message has been sent.
 * It returns a LibfabricActionResult which tell to the poll() function if it need to continue polling
 * or if it needs to return.
**/
void LibfabricConnection::rdmaReadv(int destinationEpId, struct iovec * iov, int count, LibfabricAddr remoteAddr, uint64_t remoteKey, std::function<LibfabricActionResult(void)> postAction)
{
	//check
	assert(iov != NULL);
	assert(count > 0);

	//debug
	IOC_DEBUG_ARG("libfabric:rdma", "Start RDMA read: dest=%1, iov=%2, remoteAddr=%3, count=%4")
		.arg(destinationEpId)
		.arg(iov)
		.arg(remoteAddr)
		.arg(count)
		.end();

	//go
	void ** mrDesc = new void*[count];
	for (int i = 0 ; i < count ; i++) {
		fid_mr * mr = lfDomain->getFidMR(iov[i].iov_base,iov[i].iov_len);
		assert(mr != NULL);
		mrDesc[i] = fi_mr_desc(mr);
	}

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	//do action and retry while we got FI_EAGAIN error
	int ret = 0;
	do {
		ret = fi_readv(ep, iov, mrDesc, count, it->second, (uint64_t)remoteAddr, remoteKey, new LibfabricPostActionFunction(postAction));
		if (ret == -FI_EAGAIN)
			this->pollAllCqInCache();
	} while(ret == -FI_EAGAIN);
	LIBFABRIC_CHECK_STATUS("fi_readv", ret);

	//clear tmp
	delete [] mrDesc;
}

/****************************************************/
/**
 * Start a rdma write operation to send data from the remote server.
 * This implementation consider the case where the local memory is spreaded
 * over multiple segments described by the IOv array.
 * @param destrinationEpId ID of the destination.
 * @param iov The local io vector describing the list of segments containing the data to send.
 * @param count The number of local segments in the iov.
 * @param remoteAddr The remote address to write to.
 * @param remoteKey The remote key to be allowed to read the remote segment.
 * @param size the size to read.
 * @param postAction A lambda function without parameters to be called when the message has been sent.
 * It returns a LibfabricActionResult which tell to the poll() function if it need to continue polling
 * or if it needs to return.
**/
void LibfabricConnection::rdmaWritev(int destinationEpId, struct iovec * iov, int count, LibfabricAddr remoteAddr, uint64_t remoteKey, std::function<LibfabricActionResult(void)> postAction)
{
	//checks
	assert(iov != NULL);
	assert(count > 0);

	//debug
	IOC_DEBUG_ARG("libfabric:rdma", "Start RDMA write: dest=%1, iov=%2, remoteAddr=%3, count=%4")
		.arg(destinationEpId)
		.arg(iov)
		.arg(remoteAddr)
		.arg(count)
		.end();

	//get mr
	void ** mrDesc = new void*[count];
	for (int i = 0 ; i < count ; i++) {
		fid_mr * mr = lfDomain->getFidMR(iov[i].iov_base,iov[i].iov_len);
		assert(mr != NULL);
		mrDesc[i] = fi_mr_desc(mr);
	}

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	//do action and retry while we got FI_EAGAIN error
	int ret = 0;
	do {
		ret = fi_writev(ep, iov, mrDesc, count, it->second, (uint64_t)remoteAddr, remoteKey, new LibfabricPostActionFunction(postAction));
		if (ret == -FI_EAGAIN)
			this->pollAllCqInCache();
	} while(ret == -FI_EAGAIN);
	LIBFABRIC_CHECK_STATUS("fi_writev", ret);

	//clear tmp
	delete [] mrDesc;
}

/****************************************************/
/**
 * Start a rdma write operation to send data to the remote server.
 * @param destrinationEpId ID of the destination.
 * @param localAddr The local address where to read the data from.
 * @param remoteAddr The remote address where to place the data.
 * @param remoteKey The remote key to be allowed to read the remote segment.
 * @param size the size to read.
 * @param postAction A lambda function without parameters to be called when the message has been sent.
 * It returns a LibfabricActionResult which tell to the poll() function if it need to continue polling
 * or if it needs to return.
**/
void LibfabricConnection::rdmaWrite(int destinationEpId, void * localAddr, LibfabricAddr remoteAddr, uint64_t remoteKey, size_t size, std::function<LibfabricActionResult(void)> postAction)
{
	//checks
	assert(localAddr != NULL);
	assert(size > 0);

	//debug
	IOC_DEBUG_ARG("libfabric:rdma", "Start RDMA write: dest=%1, localAddr=%2, remoteAddr=%3, size=%4")
		.arg(destinationEpId)
		.arg(localAddr)
		.arg(remoteAddr)
		.arg(size)
		.end();

	//get mr
	fid_mr * mr = lfDomain->getFidMR(localAddr,size);
	void * mrDesc = fi_mr_desc(mr);
	assert(mr != NULL);

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	//do action and retry while we got FI_EAGAIN error
	int ret = 0;
	do {
		ret = fi_write(ep, localAddr, size, mrDesc, it->second, (uint64_t)remoteAddr, remoteKey, new LibfabricPostActionFunction(postAction));
		if (ret == -FI_EAGAIN)
			this->pollAllCqInCache();
	} while(ret == -FI_EAGAIN);
	LIBFABRIC_CHECK_STATUS("fi_write", ret);
}

/****************************************************/
/**
 * Perform the polling action to wait for events. Depending on the passivPolling it will
 * perform a passive or active polling.
 * The loop will automatically exit when one of the callback attached to the operations
 * will return LF_WAIT_LOOP_UNBLOCK. If the callback return LF_WAIT_LOOP_KEEP_WAITING the
 * polling will continue to perform.
 * @param waitMsg Permit to say poll() to exit after making the first check. This as
 * only sens when using with active polling to make other operation and check regularly
 * the progress from the caller.
**/
void LibfabricConnection::poll(bool waitMsg)
{
	//vars
	fi_cq_msg_entry entry;

	//poll
	for (;;) {
		int status = pollForCompletion(this->cq, &entry, this->passivePolling);
		if (status == 1) {
			if (entry.flags & FI_RECV) {
				if (disableReceive == false)
					if (this->onRecv((size_t)entry.op_context))
						break;
			} else if (entry.op_context != IOC_LF_NO_WAKEUP_POST_ACTION) {
				LibfabricPostAction * action = (LibfabricPostAction*)entry.op_context;
				LibfabricActionResult status = action->runPostAction();
				delete action;
				if (status == LF_WAIT_LOOP_UNBLOCK)
					break;
			}
		}
		if (!waitMsg)
			break;
	}
}

/****************************************************/
bool LibfabricConnection::pollMessage(LibfabricRemoteResponse & response, LibfabricMessageType expectedMessageType)
{
	//vars
	fi_cq_msg_entry entry;

	//debug
	IOC_DEBUG_ARG("libfabric:msg", "Wait message: type=%1")
		.arg(expectedMessageType)
		.end();

	//fill default
	response.lfClientId = -1;
	response.header = NULL;
	response.message = NULL;
	response.msgBufferId = -1;

	//poll
	for (;;) {
		int status = pollForCompletion(this->cq, &entry, this->passivePolling);
		if (status == 1) {
			if (entry.flags & FI_RECV) {
				bool status = this->onRecvMessage(response, (size_t)entry.op_context);
				if (status) {
					assumeArg(response.message->header.msgType == expectedMessageType, "Got an invalide message type (%1) where %2 is expected")
						.arg(response.message->header.msgType)
						.arg(expectedMessageType)
						.end();
					return true;
				} else {
					return false;
				}
				
			} else if (entry.op_context != IOC_LF_NO_WAKEUP_POST_ACTION) {
				IOC_DEBUG_ARG("libfabric:msg", "Get message: type=%1")
					.arg(expectedMessageType)
					.end();
				LibfabricPostAction * action = (LibfabricPostAction*)entry.op_context;
				LibfabricActionResult status = action->runPostAction();
				delete action;
				if (status == LF_WAIT_LOOP_UNBLOCK)
					return false;
			}
		}
	}
}

/****************************************************/
/**
 * Wait to recive a data (not used anymore).
**/
bool LibfabricConnection::pollRx(void)
{
	//vars
	fi_cq_msg_entry entry;

	//poll
	int status = pollForCompletion(this->cq, &entry, this->passivePolling);
	if (status == 1) {
		printf("ENTRY RECV FLAG: %lu == %llu == %llu\n", entry.flags, FI_RECV, FI_SEND);
		this->onRecv((size_t)entry.op_context);
		return true;
	} else {
		return false;
	}
}

/****************************************************/
/**
 * Wait for a RDMA operation to terminate.
**/
bool LibfabricConnection::pollTx(void)
{
	fi_cq_msg_entry entry;
	int status = pollForCompletion(this->cq, &entry, this->passivePolling);
	if (status == 1) {
		printf("ENTRY SENT FLAG: %lu\n", entry.flags);
		this->onSent((void*)entry.op_context);
		return true;
	} else {
		return false;
	}
}

/****************************************************/
/**
 * @todo Remove
 * No implemented stays from old code to be removed.
 * @deprecated
**/
void LibfabricConnection::onSent(void * buffer)
{
	//check
	assert(buffer != NULL);

	//convert
	//LibfabricMessage * message = (LibfabricMessage *)buffer;

	//check

}

/****************************************************/
/**
 * Check the authentication information from a reicved message.
 * @param message The message which has been recived.
 * @param lfClientId The libfabric client ID to know to who send an error message in 
 * case of failure.
 * @param id Id of the recive buffer to be reposted in case of authentication failure.
 * @return True in case of succes, false otherwise (and a message will automatically be
 * send to the client).
**/
bool LibfabricConnection::checkAuth(LibfabricMessage * message, uint64_t lfClientId, int id)
{
	//checks
	assert(message != NULL);

	//no auth
	if (this->checkClientAuth == false)
		return true;
	
	//check
	bool ok = this->clientRegistry.checkIdentification(message->header.tcpClientId, message->header.tcpClientKey);

	//handle cases
	if (ok) {
		return true;
	} else {
		//info
		IOC_WARNING_ARG("Encounter wrong auth: ID = %1, KEY = %2")
					.arg(message->header.tcpClientId)
					.arg(message->header.tcpClientKey)
					.end();

		//send message
		LibfabricMessage * msg = new LibfabricMessage;
		fillProtocolHeader(msg->header, IOC_LF_MSG_BAD_AUTH);

		//send message
		this->sendMessage(msg, sizeof (*msg), lfClientId, [msg](void){
			delete msg;
			return LF_WAIT_LOOP_UNBLOCK;
		});

		//not good
		return false;
	}
}

/****************************************************/
/**
 * Function to be called when a message is recived by the poll() function.
 * @param id ID of the recive buffer where the message has been recived.
**/
LibfabricActionResult LibfabricConnection::onRecv(size_t id)
{
	//check
	assert(id < this->recvBuffersCount);

	//cast
	void * buffer =  this->recvBuffers[id];
	LibfabricMessage * message = (LibfabricMessage *)buffer;

	//build struct
	LibfabricClientRequest request = {
		.lfClientId = message->header.lfClientId,
		.msgBufferId = id,
		.header = &message->header,
		.message = message,
	};

	//switch
	switch(message->header.msgType) {
		case IOC_LF_MSG_CONNECT_INIT:
			onConnInit(message);
			repostReceive(id);
			break;
		case IOC_LF_MSG_BAD_AUTH:
			if (this->hookOnBadAuth) {
				LibfabricActionResult res = this->hookOnBadAuth();
				repostReceive(id);
				return res;
			} else {
				IOC_FATAL_ARG("Invalid authentification while exchanging with server, have ID = %1, KEY = %2 !")
					.arg(tcpClientId)
					.arg(tcpClientKey)
					.end();
			}
			break;
		case IOC_LF_MSG_FATAL_ERROR:
			{
				//find handler
				auto it = this->hooks.find(message->header.msgType);

				//handle
				if (it != this->hooks.end())
					return it->second->onMessage(this, request);
				else
					IOC_FATAL_ARG("Server has send error message !\n%s").arg((char*)&message->data).end();
				break;
			}
		default:
			{
				//check auth
				if (this->checkAuth(message, message->header.lfClientId, id) == false)
					return LF_WAIT_LOOP_UNBLOCK;

				//find handler
				auto it = this->hooks.find(message->header.msgType);

				//handle
				if (it != this->hooks.end())
					return it->second->onMessage(this, request);
				else
					IOC_FATAL_ARG("Invalid message type %1").arg(message->header.msgType).end();
				break;
			}
	}

	//continue
	return LF_WAIT_LOOP_KEEP_WAITING;
}

/****************************************************/
/**
 * Function to be called when a message is recived by the pollMessage() function.
 * @param response Reference to the response struct to be filled back when the message has been received.
 * @param id ID of the recive buffer where the message has been recived.
 * @return True if we get a message false otherwise. Caution, it does not check the type of message,
 * the responsability is left to the caller. It just check the eventuell auth.
**/
bool LibfabricConnection::onRecvMessage(LibfabricRemoteResponse & response, size_t id)
{
	//check
	assert(id < this->recvBuffersCount);

	//cast
	void * buffer =  this->recvBuffers[id];
	LibfabricMessage * message = (LibfabricMessage *)buffer;

	//switch
	if (message->header.msgType == IOC_LF_MSG_BAD_AUTH) {
		if (this->hookOnBadAuth) {
			this->hookOnBadAuth();
			repostReceive(id);
			return false;
		} else {
			IOC_FATAL_ARG("Invalid authentification while exchanging with server, have ID = %1, KEY = %2 !")
				.arg(tcpClientId)
				.arg(tcpClientKey)
				.end();
			return false;
		}
	} else if (message->header.msgType == IOC_LF_MSG_FATAL_ERROR) {
		IOC_FATAL_ARG("Server has send error message !\n%1").arg((char*)&message->data).end();
		return false;
	} else {
		//check auth
		if (this->checkAuth(message, message->header.lfClientId, id) == false)
			return false;
		
		//fill the struct
		response.header = &message->header;
		response.message = message;
		response.lfClientId = message->header.lfClientId;
		response.msgBufferId = id;

		//finish
		return true;
	}
}

/****************************************************/
/**
 * Attach a hook to be called when reciving a message.
 * @param messageType The type of message to filter.
 * @param hook Pointer to the hook to assocaite.
**/
void LibfabricConnection::registerHook(int messageType, Hook * hook)
{
	//check
	assert(hook != NULL);

	//debug
	IOC_DEBUG_ARG("libfabric:hook", "Register hook: type=%1")
		.arg(messageType)
		.end();

	//check if already exist and delete
	auto it = this->hooks.find(messageType);
	if (it == this->hooks.end()) {
		this->hooks[messageType] = hook;
	} else {
		delete it->second;
		it->second = hook;
	}
}

/****************************************************/
/**
 * Register a lamda functon to be called when a message arrive with a given ID.
 * @param messageType Type of message to which attach the given lambda function;
 * @param function The lambda function to call on message recive. Its parameters
 * represent:
 *   - A pointer to the libfabric connection
 *   - The client ID.
 *   - The revice buffer ID.
 *   - The buffer address.
**/
void LibfabricConnection::registerHook(int messageType, HookLambdaDef function)
{
	//allocate
	Hook * hook = new HookLambdaFunction(function);
	this->registerHook(messageType, hook);
}

/****************************************************/
/**
 * Remove a hook which has been previously registered.
 * @param messageType Define the type of message for which we want to remove
 * the hook.
**/
void LibfabricConnection::unregisterHook(int messageType)
{
	//debug
	IOC_DEBUG_ARG("libfabric:hook", "Unregister hook: type=%1")
		.arg(messageType)
		.end();
	this->hooks.erase(messageType);
}

/****************************************************/
/**
 * Function to be called on connection initialization.
 * @param message The first message recived to establish the connection.
**/
void LibfabricConnection::onConnInit(LibfabricMessage * message)
{
	//check
	assert(message != NULL);
	assert(message->header.msgType == IOC_LF_MSG_CONNECT_INIT);

	//assign id
	uint64_t epId = this->nextEndpointId++;

	//insert server address in address vector
	int err = fi_av_insert(this->av, message->data.firstClientMessage.addr, 1, &this->remoteLiAddr[epId], 0, NULL);
	if (err != 1) {
		LIBFABRIC_CHECK_STATUS("fi_av_insert", -1);
	}

	//send message
	LibfabricMessage * msg = new LibfabricMessage;
	memset(msg, 0, sizeof(*msg));
	msg->header.msgType = IOC_LF_MSG_ASSIGN_ID;
	msg->header.lfClientId = epId;
	msg->data.firstHandshakeResponse.protocolVersion = IOC_LF_PROTOCOL_VERSION;

	//debug
	IOC_DEBUG_ARG("libfabric:client", "Recive client in libfabric lfId=%1, epId=%2")
		.arg(message->header.lfClientId)
		.arg(epId)
		.end();

	//send response
	this->sendMessage(msg, sizeof (*msg), epId, [msg](void){
		delete msg;
		return LF_WAIT_LOOP_KEEP_WAITING;
	});

	//notify
	if (this->hookOnEndpointConnect)
		this->hookOnEndpointConnect(epId);
}

/****************************************************/
/**
 * In case of ressource starvation we might want to poll all the
 * completion queue entries in the cache so we let ressource for the rest
 * of the application.
 * @remark This might solve an issue we encouter when scaling to 2048 clients
 * being stopped on fi_read/fi_write with FI_EAGAIN.
 * @todo Can use a more performance structure to store cache and avoid memory allocations.
**/
void LibfabricConnection::pollAllCqInCache(void)
{
	//vars
	struct fi_cq_msg_entry entry;

	//loop while we have entries
	while(pollForCompletion(this->cq, &entry, false, false) == 1) {
		//push to list
		this->cqEntries.push_back(entry);

		//warn
		if (this->cqEntries.size() == 1000)
			IOC_WARNING_ARG("Start to have lots (1000) os pending messaged in the cqEntry cache, this might be a problem to study !");
	}
}

/****************************************************/
/**
 * Poll the libfabric completion queue to get a completion.
 * @param cq The completion queue to poll.
 * @param entry The completion entry to fill on event recive.
 * @param passivePolling Use passive or active polling. On passive polling the function
 * @param acceptCache Allow taking event from the completion cache.
 * will block waiting an event. On active polling it will return on the first
 * check so the caller need to establish the waiting loop.
**/
int LibfabricConnection::pollForCompletion(struct fid_cq * cq, struct fi_cq_msg_entry* entry, bool passivePolling, bool acceptCache)
{
	//vars
	struct fi_cq_msg_entry localEntry;
	int ret;

	//check
	assert(cq != NULL);
	assert(entry != NULL);

	//has no entry
	if (entry == NULL)
		entry = &localEntry;

	//remove from internal cache
	if (acceptCache == true && this->cqEntries.empty() == false) {
		*entry = this->cqEntries.front();
		this->cqEntries.pop_front();
		return 1;
	}

	//active or passive
	if (passivePolling)
		ret = fi_cq_sread(cq, entry, 1, NULL, -1);
	else
		ret = fi_cq_read(cq, entry, 1);

	//has one
	if (ret > 0) {
		//fprintf(stderr,"cq_read = %d\n", ret);
		return 1;
	} else if (ret != -FI_EAGAIN) {
		struct fi_cq_err_entry err_entry;
		fi_cq_readerr(cq, &err_entry, 0);
		IOC_WARNING_ARG("fi_cq_read error : %1, %2")
			.arg(fi_strerror(err_entry.err))
			.arg(fi_cq_strerror(cq, err_entry.prov_errno, err_entry.err_data, NULL, 0))
			.end();
		return 0;
	} else {
		return 0;
	}
}

/****************************************************/
/**
 * Define various hooks to be called on events.
 * @param hookOnEndpointConnect Provide a lambda function to be called on incomming connection.
**/
void LibfabricConnection::setHooks(std::function<void(int)> hookOnEndpointConnect)
{
	this->hookOnEndpointConnect = hookOnEndpointConnect;
}

/****************************************************/
/**
 * Setup the TCP connection info, meaning the TCP client ID and the TCP client KEY
 * to we can send them for every message we send to the server so he can check
 * out identifty as there is no connection tracking with rxm libfabric driver.
 * @param tcpClientId The TCP client ID.
 * @param tcpClientKey The key to prove we are the right client.
**/
void LibfabricConnection::setTcpClientInfos(uint64_t tcpClientId, uint64_t tcpClientKey)
{
	this->tcpClientId = tcpClientId;
	this->tcpClientKey = tcpClientKey;
}

/****************************************************/
/**
 * Helper function to fill the protocl header struct, mainly the lcient authentifiants
 * and the message type.
 * @param header Reference to the header struct to fill.
 * @param type Tye of msesage to be sent.
**/
void LibfabricConnection::fillProtocolHeader(LibfabricMessageHeader & header, uint64_t type)
{
	header.msgType = type;
	header.lfClientId = this->clientId;
	header.tcpClientId = this->tcpClientId;
	header.tcpClientKey = this->tcpClientKey;
}

/****************************************************/
/**
 * @return Return a reference to the client registry for unit tests.
**/
ClientRegistry & LibfabricConnection::getClientRegistry(void)
{
	return clientRegistry;
}

/****************************************************/
/**
 * Enable of disable the authentication checking on message recieve.
 * @param value Set to true to enable the checking, false to disable.
**/
void LibfabricConnection::setCheckClientAuth(bool value)
{
	this->checkClientAuth = value;
}

/****************************************************/
/**
 * Setup a hook to be called when getting an authentication failure so we can print
 * a warning on the server output.
 * @param hookOnBadAuth Lambda function to be called on the given event.
**/
void LibfabricConnection::setOnBadAuth(std::function<LibfabricActionResult(void)> hookOnBadAuth)
{
	this->hookOnBadAuth = hookOnBadAuth;
}

/****************************************************/
/**
 * Build a response message and send it to the client.
 * @param msgType Define the type of message to be used.
 * @param lfClientId Define the libfabric ID of the client to which to send the message.
 * @param status The status code to put in the response.
 * @param unblock For unit test we need to unblock the polling but not on the server implementation (default).
**/
void LibfabricConnection::sendResponse(LibfabricMessageType msgType, uint64_t lfClientId, int32_t status, bool unblock)
{
	//prepare message
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.msgType = msgType;
	msg->header.lfClientId = lfClientId;
	msg->data.response.status = status;
	msg->data.response.msgDataSize = 0;
	msg->data.response.msgHasData = false;

	//send message
	this->sendMessage(msg, sizeof (*msg), lfClientId, [msg, unblock](void){
		delete msg;
		if (unblock)
			return LF_WAIT_LOOP_UNBLOCK;
		else
			return LF_WAIT_LOOP_KEEP_WAITING;
	});
}

/****************************************************/
/**
 * Build a response message and send it to the client with attached data.
 * @param msgType Define the type of message to be used.
 * @param lfClientId Define the libfabric ID of the client to which to send the message.
 * @param status The status code to put in the response.
 * @param data Address of the data to concat at the end of the message.
 * @param size Size of the data buffer to be added to the message
 * @param unblock For unit test we need to unblock the polling but not on the server implementation (default).
**/
void LibfabricConnection::sendResponse(LibfabricMessageType msgType, uint64_t lfClientId, int32_t status, const char * data, size_t size, bool unblock)
{
	//prepare message
	size_t bufferSize = sizeof(LibfabricMessage)+size;
	void * buffer = malloc(bufferSize);
	LibfabricMessage * msg = reinterpret_cast<LibfabricMessage*>(buffer);
	msg->header.msgType = msgType;
	msg->header.lfClientId = lfClientId;
	msg->data.response.status = status;
	msg->data.response.msgDataSize = size;
	msg->data.response.msgHasData = true;
	memcpy(msg->extraData, data, size);

	//send message
	this->sendMessage(msg, bufferSize, lfClientId, [buffer, unblock](void){
		free(buffer);
		if (unblock)
			return LF_WAIT_LOOP_UNBLOCK;
		else
			return LF_WAIT_LOOP_KEEP_WAITING;
	});
}

/****************************************************/
/**
 * Build a response message and send it to the client with attached data build from several fragments.
 * @param msgType Define the type of message to be used.
 * @param lfClientId Define the libfabric ID of the client to which to send the message.
 * @param status The status code to put in the response.
 * @param fragments Array of fragments to concatenate in the message.
 * @param cntFragments Number of fragments to consider.
 * @param unblock For unit test we need to unblock the polling but not on the server implementation (default).
**/
void LibfabricConnection::sendResponse(LibfabricMessageType msgType, uint64_t lfClientId, int32_t status, const LibfabricBuffer * fragments, size_t cntFragments, bool unblock)
{
	//check
	assert(fragments != NULL);

	//calc
	size_t sumSize = 0;
	for (size_t i = 0 ; i < cntFragments ; i++)
		sumSize += fragments[i].size;

	//prepare message
	size_t bufferSize = sizeof(LibfabricMessage) + sumSize;
	void * buffer = malloc(bufferSize);
	LibfabricMessage * msg = reinterpret_cast<LibfabricMessage*>(buffer);
	msg->header.msgType = msgType;
	msg->header.lfClientId = lfClientId;
	msg->data.response.status = status;
	msg->data.response.msgDataSize = sumSize;
	msg->data.response.msgHasData = true;

	//copy all & concat
	char * cursor = msg->extraData;
	for (size_t i = 0 ; i < cntFragments ; i++) {
		memcpy(cursor, fragments[i].buffer, fragments[i].size);
		cursor += fragments[i].size;
	}

	//send message
	this->sendMessage(msg, bufferSize, lfClientId, [buffer, unblock](void){
		free(buffer);
		if (unblock)
			return LF_WAIT_LOOP_UNBLOCK;
		else
			return LF_WAIT_LOOP_KEEP_WAITING;
	});
}

}
