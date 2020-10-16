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
//local
#include "LibfabricConnection.hpp"
#include "../common/Debug.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
LibfabricConnection::LibfabricConnection(LibfabricDomain * lfDomain, bool wait)
{
	//check
	assert(lfDomain != NULL);

	//set
	this->lfDomain = lfDomain;
	this->wait = wait;
	this->recvBuffersCount = 0;
	this->recvBuffers = NULL;
	this->nextEndpointId = 0;
	this->recvBuffersSize = 0;

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
	cq_attr.format = FI_CQ_FORMAT_CONTEXT;
	cq_attr.size = fi->tx_attr->size;
	if (wait)
		cq_attr.wait_obj = FI_WAIT_UNSPEC;
	else
		cq_attr.wait_obj = FI_WAIT_NONE;
	//printf("CQ_SIZE: %ld\n", cq_attr.size);

	//setup cq read
	err = fi_cq_open(domain, &cq_attr, &this->tx_cq, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_cq_open",err);
	
	//setup attr
	cq_attr.size = fi->rx_attr->size;

	//setup cq read
	err = fi_cq_open(domain, &cq_attr, &this->rx_cq, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_cq_open",err);

	//addres vector
	//av_attr.type = FI_AV_MAP;
	err = fi_av_open(domain, &av_attr, &this->av, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_av_open",err);

	//create endpoint
	err = fi_endpoint(domain, fi, &this->ep, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_endpoint",err);

	//bind completion queue
	err = fi_ep_bind(this->ep, (struct fid *)this->tx_cq, FI_SEND);
	LIBFABRIC_CHECK_STATUS("fi_ep_bind",err);
	err = fi_ep_bind(this->ep, (struct fid *)this->rx_cq, FI_RECV);
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
	fi_close((fid_t)tx_cq);
	fi_close((fid_t)rx_cq);

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
	msg->type = IOC_LF_MSG_CONNECT_INIT;
	err = fi_getname(&this->ep->fid, msg->data.addr, &addrlen);
	LIBFABRIC_CHECK_STATUS("fi_getname", err);
	assert(addrlen <= IOC_LF_MAX_ADDR_LEN);

	//send
	this->sendMessage(msg, sizeof(LibfabricMessage), IOC_LF_SERVER_ID);
}

/****************************************************/
void LibfabricConnection::sendMessage(void * buffer, size_t size, int destinationEpId)
{
	//vars
	int err;

	//checks
	assert(size < recvBuffersSize);

	//search
	auto it = this->remoteLiAddr.find(destinationEpId);
	assumeArg(it != this->remoteLiAddr.end(), "Client endpoint id not found : %1")
		.arg(destinationEpId)
		.end();

	//send
	do {
		err = fi_send(this->ep, buffer, size, NULL, it->second, buffer);
	} while(err == -FI_EAGAIN);
	LIBFABRIC_CHECK_STATUS("fi_send", err);
}

/****************************************************/
void LibfabricConnection::poll(void)
{
	pollRx();
	pollTx();
}

/****************************************************/
bool LibfabricConnection::pollRx(void)
{
	void * context = NULL;
	int status = pollForCompletion(this->rx_cq, &context, this->wait);
	if (status == 1) {
		this->onRecv((size_t)context);
		return true;
	} else {
		return false;
	}
}

/****************************************************/
bool LibfabricConnection::pollTx(void)
{
	return false;
}

/****************************************************/
void LibfabricConnection::onRecv(size_t id)
{
	//check
	assert(id < this->recvBuffersCount);

	//cast
	char * buffer =  this->recvBuffers[id];
	LibfabricMessage * message = (LibfabricMessage *)buffer;

	//switch
	switch(message->type) {
		case IOC_LF_MSG_CONNECT_INIT:
			onConnInit(message);
			repostRecive(id);
			break;
		case IOC_LF_MSG_ASSIGN_ID:
			break;
		default:
			IOC_FATAL_ARG("Invalid message type %1").arg(message->type).end();
			break;
	}
}

/****************************************************/
void LibfabricConnection::onConnInit(LibfabricMessage * message)
{
	//check
	assert(message != NULL);
	assert(message->type == IOC_LF_MSG_CONNECT_INIT);

	//assign id
	int epId = this->nextEndpointId++;

	//insert server address in address vector
	int err = fi_av_insert(this->av, message->data.addr, 1, &this->remoteLiAddr[epId], 0, NULL);
	if (err != 1) {
		LIBFABRIC_CHECK_STATUS("fi_av_insert", -1);
	}

	//notify
	this->hookOnEndpointConnect(epId);
}

/****************************************************/
int LibfabricConnection::pollForCompletion(struct fid_cq * cq, void ** context, bool wait)
{
	struct fi_cq_entry entry;
	int ret;

	if (wait)
		ret = fi_cq_sread(cq, &entry, 1, NULL, -1);
	else
		ret = fi_cq_read(cq, &entry, 1);
	if (ret > 0) {
		//fprintf(stderr,"cq_read = %d\n", ret);
		if (context != NULL)
			*context = entry.op_context;
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

}
