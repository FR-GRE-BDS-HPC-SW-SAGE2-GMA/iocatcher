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
//tmp
#include <unistd.h>
#include <sys/mman.h>
//local
#include "LibfabricDomain.hpp"
#include "../common/Debug.hpp"
//libfabric
#include <rdma/fi_rma.h>

/****************************************************/
namespace IOC
{

/****************************************************/
LibfabricDomain::LibfabricDomain(const std::string & serverIp, const std::string & port, bool isDomainServer)
{
	//check
	assert(serverIp.empty() == false);
	assert(port.empty() == false);

	//allocate fi
	struct fi_info *hints;
	hints = fi_allocinfo();
	if (!hints) {
		printf("fi_allocinfo failed\n");
		exit(1);
	}
	hints->caps = FI_MSG; // | FI_RMA_PMEM; //|FI_RMA;
	//hints->caps = FI_MSG;
	//hints->mode = FI_CONTEXT | FI_RX_CQ_DATA;
	//hints->mode = FI_LOCAL_MR;
	hints->mode = FI_MR_MMU_NOTIFY;
	//hints->domain_attr->mr_mode = FI_MR_LOCAL;
	hints->ep_attr->type = FI_EP_RDM;
	//hints->fabric_attr->prov_name = strdup("verbs;ofi_rxm");

	//get fi_info
	int err;
	if (isDomainServer)
		err = fi_getinfo(FI_VERSION(1,11), serverIp.c_str(), port.c_str(), FI_SOURCE, hints, &this->fi);
	else
		err = fi_getinfo(FI_VERSION(1,11), serverIp.c_str(), port.c_str(), 0, hints, &this->fi);
	LIBFABRIC_CHECK_STATUS("fi_getinfo",err);
	//printf("MSG: %d\n", this->fi->caps & FI_MSG);
	//printf("RMA: %d\n", this->fi->caps & FI_RMA);
	//printf("RMA_PMEM: %d\n", this->fi->caps & FI_RMA_PMEM);

	//display
	//IOC_INFO_ARG("NET: %1").arg(fi->fabric_attr->prov_name).end();

	//IOC_
	err = fi_fabric(fi->fabric_attr, &this->fabric, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_fabric",err);

	//setup domain
	err = fi_domain(fabric, fi, &this->domain, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_domain",err);

	//get mr mode
	this->virtMrMode = true;
	if (strncmp(fi->fabric_attr->prov_name, "tcp;", 4) == 0)
		this->virtMrMode = false;
}

/****************************************************/
LibfabricDomain::~LibfabricDomain(void)
{
	//free msg buffers
	this->msgBuffersMutex.lock();
	for (auto it : this->msgBuffers) {
		this->unregisterSegment(it, this->msgBufferSize);
		free(it);
	}
	this->msgBuffers.clear();
	this->msgBuffersMutex.unlock();

	fi_close(&domain->fid);
	fi_close(&fabric->fid);
	fi_freeinfo(fi);
}

/****************************************************/
const char * LibfabricDomain::getLFProviderName(void) const
{
	return this->fi->fabric_attr->prov_name;
}

/****************************************************/
struct fi_info * LibfabricDomain::getFiInfo(void)
{
	return this->fi;
}

/****************************************************/
struct fid_fabric * LibfabricDomain::getFabric(void)
{
	return this->fabric;
}

/****************************************************/
struct fid_domain * LibfabricDomain::getDomain(void)
{
	return this->domain;
}

/****************************************************/
Iov LibfabricDomain::registerSegment(void * ptr, size_t size, bool read, bool write, bool pmem)
{
	MemoryRegion region;
	region.ptr = ptr;
	region.size = size;
	region.mr = nullptr;

	//access
	uint64_t accessFlag = 0;
	if (read)
		accessFlag |= FI_REMOTE_READ;
	if (write)
		accessFlag |= FI_REMOTE_WRITE;

	//flag
	uint64_t flags = 0;
	if (pmem)
		flags |= FI_RMA_PMEM;

	//keep key tracking
	static int cnt = 0;

	//lock
	segmentMutex.lock();

	//reg into ofi
	int ret = fi_mr_reg(domain, ptr, size, accessFlag, flags, cnt++/*TOTO*/, 0, &(region.mr), nullptr);
	LIBFABRIC_CHECK_STATUS("fi_mr_reg",ret);

	//update page table
	/*struct iovec iov2;
	iov2.iov_base = ptr;
	iov2.iov_len = size;
	ret = fi_mr_refresh(region.mr, &iov2, 1, 0);
	LIBFABRIC_CHECK_STATUS("fi_mr_refresh",ret);*/

	segments.push_back(region);
	segmentMutex.unlock();

	Iov iov;
	iov.addr = (virtMrMode) ? ptr : 0;
	iov.key = fi_mr_key(region.mr);

	return iov;
}

/****************************************************/
void LibfabricDomain::unregisterSegment(void * ptr, size_t size)
{
	fid_mr* mr = getFidMR(ptr,size);
	fi_close(&mr->fid);

	//remove from list
	segmentMutex.lock();
	for (auto it = segments.begin() ; it != segments.end() ; ++it) {
		//printf("Search %p => %p - %lu\n", ptr, it->ptr, it->size);
		if (it->ptr <= ptr && (char*)it->ptr + it->size > ptr) {
			segments.erase(it);
			break;
		}
	}
	segmentMutex.unlock();
}

/****************************************************/
fid_mr* LibfabricDomain::getFidMR ( void* ptr, size_t size )
{
	MemoryRegion * region = getMR(ptr,size);
	if (region == nullptr)
		return nullptr;
	else
		return region->mr;
}

/****************************************************/
MemoryRegion* LibfabricDomain::getMR ( void* ptr, size_t size )
{
	//search
	MemoryRegion * mr = nullptr;
	segmentMutex.lock();
	for (auto it = segments.begin() ; it != segments.end() ; ++it) {
		//printf("Search %p => %p - %lu\n", ptr, it->ptr, it->size);
		if (it->ptr <= ptr && (char*)it->ptr + it->size > ptr) {
			if ((char*)ptr+size > (char*)it->ptr + it->size)
				DAQ_WARNING("Caution, a segment from libfabric not completetly fit with the request which is larger.");
			mr = &(*it);
			break;
		}
	}
	segmentMutex.unlock();
	return mr;
}

/****************************************************/
void LibfabricDomain::setMsgBuffeSize(size_t size)
{
	this->msgBufferSize = size;
}

/****************************************************/
void * LibfabricDomain::getMsgBuffer(void)
{
	//vars
	void * res = NULL;

	//critical section
	{
		//lock
		msgBuffersMutex.lock();

		//check if need to allocate one
		if (this->msgBuffers.empty()) {
			assume(this->msgBufferSize != 0, "You need to configure message buffer size of libfabric domain !");
			res = malloc(this->msgBufferSize);
			this->registerSegment(res, this->msgBufferSize, true, false, false);
		} else {
			res = this->msgBuffers.back();
			this->msgBuffers.pop_back();
		}

		//unlock
		msgBuffersMutex.unlock();
	}

	//ret
	return res;
}

/****************************************************/
void LibfabricDomain::retMsgBuffer(void * buffer)
{
	//check
	assume(buffer != NULL, "Fail to return NULL buffer !");

	//critical section
	{
		//lock
		msgBuffersMutex.lock();

		//append
		this->msgBuffers.push_back(buffer);

		//unlock
		msgBuffersMutex.unlock();
	}
}

}
