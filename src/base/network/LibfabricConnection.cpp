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
	assert(domain != NULL);

	//set
	this->wait = wait;

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
	fi_close((fid_t)ep);
	fi_close((fid_t)av);
	fi_close((fid_t)tx_cq);
	fi_close((fid_t)rx_cq);
}

}
