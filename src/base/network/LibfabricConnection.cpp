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
#include "../common/Debug.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
void LibfabricPostAction::freeBuffer(void)
{
	if (connection != NULL)
		if (isRecv)
			connection->repostRecive(bufferId);
}

/****************************************************/
bool LibfabricPostActionFunction::runPostAction(void)
{
	return this->function();
};

/****************************************************/
LibfabricConnection::LibfabricConnection(LibfabricDomain * lfDomain, bool wait)
{
	//check
	assert(lfDomain != NULL);

	//set
	this->used = false;
	this->clientId = -1;
	this->lfDomain = lfDomain;
	this->wait = wait;
	this->recvBuffersCount = 0;
	this->recvBuffers = NULL;
	this->nextEndpointId = 0;
	this->recvBuffersSize = 0;
	this->tcpClientId = 0;
	this->tcpClientKey = 0;
	this->checkClientAuth = false;

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
	if (wait)
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
LibfabricConnection::~LibfabricConnection(void)
{
	//close
	fi_close((fid_t)ep);
	fi_close((fid_t)av);
	fi_close((fid_t)cq);

	//destroy buffers
	if (this->recvBuffers != NULL) {
		for (int i = 0 ; i < recvBuffersCount ; i++) {
			delete [] this->recvBuffers[i];
		}
		delete [] this->recvBuffers;
	}
}

/****************************************************/
void LibfabricConnection::postRecives(size_t size, int count)
{
	//setup
	this->recvBuffersCount = count;
	this->recvBuffersSize = size;

	//allocate array
	this->recvBuffers = new char * [count];

	//allocate buffers
	for (size_t i = 0 ; i < count ; i++) {
		//alloc
		this->recvBuffers[i] = new char[size];

		//register for reception
		//fid_mr * mr;
		//int err = fi_mr_reg(this->lfDomain->getDomain(), this->recvBuffers[i], size, FI_RECV, 0, 0, 0, &mr, NULL);
		//LIBFABRIC_CHECK_STATUS("fi_mr_reg", err);

		//post
		this->repostRecive(i);
	}
}

/****************************************************/
void LibfabricConnection::repostRecive(size_t id)
{
	//post
	int err = fi_recv(this->ep, this->recvBuffers[id], recvBuffersSize, 0, 0, (void*)id);
	LIBFABRIC_CHECK_STATUS("fi_recv", err);
}

/****************************************************/
void LibfabricConnection::joinServer(void)
{
	//vars
	int err;
	size_t addrlen = IOC_LF_MAX_ADDR_LEN;

	//insert server address in address vector
	err = fi_av_insert(this->av, this->lfDomain->getFiInfo()->dest_addr, 1, &this->remoteLiAddr[IOC_LF_SERVER_ID], 0, NULL);
	if (err != 1)
		LIBFABRIC_CHECK_STATUS("fi_av_insert", -1);

	//new message
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.type = IOC_LF_MSG_CONNECT_INIT;
	err = fi_getname(&this->ep->fid, msg->data.addr, &addrlen);
	LIBFABRIC_CHECK_STATUS("fi_getname", err);
	assert(addrlen <= IOC_LF_MAX_ADDR_LEN);

	//register hook
	this->registerHook(IOC_LF_MSG_ASSIGN_ID, [this](int clientId, size_t id, void * buffer) {
		//printf("get clientID %d\n", clientId);
		this->clientId = clientId;
		this->repostRecive(id);
		return true;
	});

	//send
	this->sendMessage(msg, sizeof(LibfabricMessage), IOC_LF_SERVER_ID, [msg](){
		delete msg;
		return false;
	});

	//wait send
	this->poll(true);
}

/****************************************************/
void LibfabricConnection::sendMessage(void * buffer, size_t size, int destinationEpId, std::function<bool(void)> postAction)
{
	//vars
	int err;

	//checks
	assert(size <= recvBuffersSize);

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	//send
	do {
		err = fi_send(this->ep, buffer, size, NULL, it->second, new LibfabricPostActionFunction(postAction));
	} while(err == -FI_EAGAIN);
	LIBFABRIC_CHECK_STATUS("fi_send", err);
}

/****************************************************/
void LibfabricConnection::rdmaRead(int destinationEpId, void * localAddr, void * remoteAddr, uint64_t remoteKey, size_t size, std::function<bool(void)> postAction)
{
	fid_mr * mr = lfDomain->getFidMR(localAddr,size);
	void * mrDesc = fi_mr_desc(mr);

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	int ret = fi_read(ep, localAddr, size, mrDesc, it->second, (uint64_t)remoteAddr, remoteKey, new LibfabricPostActionFunction(postAction));
	LIBFABRIC_CHECK_STATUS("fi_read", ret);
}

/****************************************************/
void LibfabricConnection::rdmaReadv(int destinationEpId, struct iovec * iov, int count, void * remoteAddr, uint64_t remoteKey, std::function<bool(void)> postAction)
{
	void ** mrDesc = new void*[count];
	for (int i = 0 ; i < count ; i++) {
		fid_mr * mr = lfDomain->getFidMR(iov[i].iov_base,iov[i].iov_len);
		mrDesc[i] = fi_mr_desc(mr);
	}

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	int ret = fi_readv(ep, iov, mrDesc, count, it->second, (uint64_t)remoteAddr, remoteKey, new LibfabricPostActionFunction(postAction));
	LIBFABRIC_CHECK_STATUS("fi_read", ret);

	//clear tmp
	delete [] mrDesc;
}

/****************************************************/
void LibfabricConnection::rdmaWritev(int destinationEpId, struct iovec * iov, int count, void * remoteAddr, uint64_t remoteKey, std::function<bool(void)> postAction)
{
	void ** mrDesc = new void*[count];
	for (int i = 0 ; i < count ; i++) {
		fid_mr * mr = lfDomain->getFidMR(iov[i].iov_base,iov[i].iov_len);
		mrDesc[i] = fi_mr_desc(mr);
	}

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	int ret = fi_writev(ep, iov, mrDesc, count, it->second, (uint64_t)remoteAddr, remoteKey, new LibfabricPostActionFunction(postAction));
	LIBFABRIC_CHECK_STATUS("fi_read", ret);

	//clear tmp
	delete [] mrDesc;
}

/****************************************************/
void LibfabricConnection::rdmaWrite(int destinationEpId, void * localAddr, void * remoteAddr, uint64_t remoteKey, size_t size, std::function<bool(void)> postAction)
{
	fid_mr * mr = lfDomain->getFidMR(localAddr,size);
	void * mrDesc = fi_mr_desc(mr);

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	int ret = fi_write(ep, localAddr, size, mrDesc, it->second, (uint64_t)remoteAddr, remoteKey, new LibfabricPostActionFunction(postAction));
	LIBFABRIC_CHECK_STATUS("fi_write", ret);
}

/****************************************************/
void LibfabricConnection::poll(bool waitMsg)
{
	//vars
	fi_cq_msg_entry entry;

	//poll
	for (;;) {
		int status = pollForCompletion(this->cq, &entry, this->wait);
		if (status == 1) {
			if (entry.flags & FI_RECV) {
				if (this->onRecv((size_t)entry.op_context))
					break;
			} else {
				LibfabricPostAction * action = (LibfabricPostAction*)entry.op_context;
				bool status = action->runPostAction();
				delete action;
				if (status)
					break;
			}
		}
		if (!waitMsg)
			break;
	}
}

/****************************************************/
bool LibfabricConnection::pollRx(void)
{
	//vars
	fi_cq_msg_entry entry;

	//poll
	int status = pollForCompletion(this->cq, &entry, this->wait);
	if (status == 1) {
		printf("ENTRY RECV FLAG: %lu == %llu == %llu\n", entry.flags, FI_RECV, FI_SEND);
		this->onRecv((size_t)entry.op_context);
		return true;
	} else {
		return false;
	}
}

/****************************************************/
bool LibfabricConnection::pollTx(void)
{
	fi_cq_msg_entry entry;
	int status = pollForCompletion(this->cq, &entry, this->wait);
	if (status == 1) {
		printf("ENTRY SENT FLAG: %lu\n", entry.flags);
		this->onSent((void*)entry.op_context);
		return true;
	} else {
		return false;
	}
}

/****************************************************/
void LibfabricConnection::onSent(void * buffer)
{
	//check
	assert(buffer != NULL);

	//convert
	LibfabricMessage * message = (LibfabricMessage *)buffer;

	//check

}

/****************************************************/
bool LibfabricConnection::checkAuth(LibfabricMessage * message, int clientId, int id)
{
	//no auth
	if (this->checkClientAuth == false)
		return true;
	
	//check
	bool ok = this->clientRegistry.checkIdentification(message->header.tcpClientId, message->header.tcpClientKey);

	//we can repost the buffer
	repostRecive(id);

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
		this->sendMessage(msg, sizeof (*msg), clientId, [msg](void){
			delete msg;
			return true;
		});

		//not good
		return false;
	}
}

/****************************************************/
bool LibfabricConnection::onRecv(size_t id)
{
	//check
	assert(id < this->recvBuffersCount);

	//cast
	void * buffer =  this->recvBuffers[id];
	LibfabricMessage * message = (LibfabricMessage *)buffer;

	//switch
	switch(message->header.type) {
		case IOC_LF_MSG_CONNECT_INIT:
			onConnInit(message);
			repostRecive(id);
			break;
		case IOC_LF_MSG_BAD_AUTH:
			if (this->hookOnBadAuth) {
				return this->hookOnBadAuth();
			} else {
				IOC_FATAL_ARG("Invalid authentification while exchanging with server, have ID = %1, KEY = %2 !")
					.arg(tcpClientId)
					.arg(tcpClientKey)
					.end();
			}
			break;
		default:
			//check auth
			if (this->checkAuth(message, message->header.clientId, id) == false)
				return true;

			//find handler
			auto it = this->hooks.find(message->header.type);

			//handle
			if (it != this->hooks.end())
				return it->second(message->header.clientId, id, buffer);
			else
				IOC_FATAL_ARG("Invalid message type %1").arg(message->header.type).end();
			break;
	}

	//continue
	return false;
}

/****************************************************/
void LibfabricConnection::registerHook(int messageType, std::function<bool(int, size_t, void*)> function)
{
	//assert(this->hooks.find(messageType) == this->hooks.end());
	this->hooks[messageType] = function;
}

/****************************************************/
void LibfabricConnection::unregisterHook(int messageType)
{
	this->hooks.erase(messageType);
}

/****************************************************/
void LibfabricConnection::onConnInit(LibfabricMessage * message)
{
	//check
	assert(message != NULL);
	assert(message->header.type == IOC_LF_MSG_CONNECT_INIT);

	//assign id
	int epId = this->nextEndpointId++;

	//insert server address in address vector
	int err = fi_av_insert(this->av, message->data.addr, 1, &this->remoteLiAddr[epId], 0, NULL);
	if (err != 1) {
		LIBFABRIC_CHECK_STATUS("fi_av_insert", -1);
	}

	//send message
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.type = IOC_LF_MSG_ASSIGN_ID;
	msg->header.clientId = epId;

	this->sendMessage(msg, sizeof (*msg), epId, [msg](void){
		delete msg;
		return false;
	});

	//notify
	if (this->hookOnEndpointConnect)
		this->hookOnEndpointConnect(epId);
}

/****************************************************/
int LibfabricConnection::pollForCompletion(struct fid_cq * cq, struct fi_cq_msg_entry* entry, bool wait)
{
	//vars
	struct fi_cq_msg_entry localEntry;
	int ret;

	//has no entry
	if (entry == NULL)
		entry = &localEntry;

	//active or passive
	if (wait)
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
void LibfabricConnection::setHooks(std::function<void(int)> hookOnEndpointConnect)
{
	this->hookOnEndpointConnect = hookOnEndpointConnect;
}

/****************************************************/
void LibfabricConnection::setTcpClientInfos(uint64_t tcpClientId, uint64_t tcpClientKey)
{
	this->tcpClientId = tcpClientId;
	this->tcpClientKey = tcpClientKey;
}

/****************************************************/
void LibfabricConnection::fillProtocolHeader(LibfabricMessageHeader & header, int type)
{
	header.type = type;
	header.clientId = this->clientId;
	header.tcpClientId = this->tcpClientId;
	header.tcpClientKey = this->tcpClientKey;
}

/****************************************************/
ClientRegistry & LibfabricConnection::getClientRegistry(void)
{
	return clientRegistry;
}

/****************************************************/
void LibfabricConnection::setCheckClientAuth(bool value)
{
	this->checkClientAuth = value;
}

/****************************************************/
void LibfabricConnection::setOnBadAuth(std::function<bool(void)> hookOnBadAuth)
{
	this->hookOnBadAuth = hookOnBadAuth;
}

}
