/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
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
#include "Protocol.hpp"
#include "../common/Debug.hpp"
//libfabric
#include <rdma/fi_rma.h>

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Establish a new libfabric domain to latter create connections in this domain.
 * @param serverIp The IP of the server to join or the IP to listen if being the server.
 * @param port The server to connect on if being client or to listen if being the server.
 * @param isDomainServer Define if this instance is the domain server or will be used to 
 * connection clients to the server.
**/
LibfabricDomain::LibfabricDomain(const std::string & serverIp, const std::string & port, bool isDomainServer)
{
	//check
	assert(serverIp.empty() == false);
	assert(port.empty() == false);

	//defaults is 1MB, can be changed by calling setMsgBuffeSize() before allocating buffers in the pool
	this->msgBufferSize = 1024*1024;

	//allocate fi
	struct fi_info *hints = fi_allocinfo();
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

	//free
	fi_freeinfo(hints);

	//display
	//IOC_INFO_ARG("NET: %1").arg(fi->fabric_attr->prov_name).end();

	//checks
	assume(this->fi->src_addrlen <= IOC_LF_MAX_ADDR_LEN, "Invalid hard coded maximum address len !");
	assume(this->fi->dest_addrlen <= IOC_LF_MAX_ADDR_LEN, "Invalid hard coded maximum address len !");

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

	//debug
	IOC_DEBUG_ARG("libfaric:domain", "Create domaine with provider '%1'").arg(getLFProviderName()).end();
}

/****************************************************/
/**
 * Destroy the domain and clean every ressources attached to it.
 * @warning Caution you need to destroy the connections first it will
 * not be done by this function.
**/
LibfabricDomain::~LibfabricDomain(void)
{
	//CRITICAL SECTION
	{
		//take lock
		std::lock_guard<std::mutex> guard(this->msgBuffersMutex);

		//free msg buffers
		for (auto it : this->msgBuffers) {
			this->unregisterSegment(it, this->msgBufferSize);
			free(it);
		}
		this->msgBuffers.clear();
	}

	//close
	fi_close(&domain->fid);
	fi_close(&fabric->fid);
	fi_freeinfo(fi);
}

/****************************************************/
/**
 * Return the libfabric provider name in use.
**/
const char * LibfabricDomain::getLFProviderName(void) const
{
	return this->fi->fabric_attr->prov_name;
}

/****************************************************/
/**
 * Return the libfabric infos in use.
**/
struct fi_info * LibfabricDomain::getFiInfo(void)
{
	return this->fi;
}

/****************************************************/
/**
 * Return the current fabric to be used by the connection to connect.
**/
struct fid_fabric * LibfabricDomain::getFabric(void)
{
	return this->fabric;
}

/****************************************************/
/**
 * Return the current domain to be used by the connection to connect.
**/
struct fid_domain * LibfabricDomain::getDomain(void)
{
	return this->domain;
}

/****************************************************/
/**
 * Register a memory segment to be used for RDMA operations.
 * @param ptr Base address of the segment.
 * @param size Size of the segment to register.
 * @param read If to be used for read operation (reading the segment).
 * @param write If to be used to write operation (writing in the segment).
 * @param pmem If being a persistent memory (pmem) segment (not sure this is really necessary).
 * @return Return the description of the segment to be transmitted to the remote server
 * making the RDMA operation.
**/
Iov LibfabricDomain::registerSegment(void * ptr, size_t size, bool read, bool write, bool pmem)
{
	//vars
	MemoryRegion region;
	region.ptr = ptr;
	region.size = size;
	region.mr = nullptr;

	//checks
	assert(ptr != nullptr);
	assert(size > 0);
	assert(getMR(ptr, size) == nullptr);

	//debug
	IOC_DEBUG_ARG("libfaric:register", "Register segment ptr=%1, size=%2")
		.arg(ptr)
		.arg(size)
		.end();

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

	//CRITICAL SECTION
	{
		//lock
		std::lock_guard<std::mutex> guard(this->segmentMutex);

		//reg into ofi
		int ret = fi_mr_reg(domain, ptr, size, accessFlag, flags, cnt++/*TOTO*/, 0, &(region.mr), nullptr);
		LIBFABRIC_CHECK_STATUS("fi_mr_reg",ret);

		//update page table
		/*struct iovec iov2;
		iov2.iov_base = ptr;
		iov2.iov_len = size;
		ret = fi_mr_refresh(region.mr, &iov2, 1, 0);
		LIBFABRIC_CHECK_STATUS("fi_mr_refresh",ret);*/

		segments[(char*)ptr+size-1] = region;
	}

	Iov iov;
	iov.addr = (virtMrMode) ? (LibfabricAddr)ptr : 0;
	iov.key = fi_mr_key(region.mr);

	return iov;
}

/****************************************************/
/**
 * Deregister the given memory segment.
 * @param ptr Base address of the segment to deregister.
 * @param size Size of the segment to deregister.
**/
void LibfabricDomain::unregisterSegment(void * ptr, size_t size)
{
	//checks
	assert(ptr != NULL);

	//debug
	IOC_DEBUG_ARG("libfaric:register", "Unregister segment ptr=%1, size=%2")
		.arg(ptr)
		.arg(size)
		.end();

	//CRITICAL SECTION
	{
		//lock
		std::lock_guard<std::mutex> guard(this->segmentMutex);

		//quick search
		auto it = segments.lower_bound(ptr);

		//check
		assumeArg(it != segments.end(),"Fail to find segment to unregister: %1 (%2).")
			.arg(ptr).arg(size).end();
		assumeArg(it->second.ptr <= ptr && (char*)it->second.ptr + it->second.size > ptr, 
			"Fail to find segment to unregister, one found does not match: %1 (%2).")
				.arg(ptr).arg(size).end();

		//close & erase
		fi_close(&it->second.mr->fid);
		segments.erase(it);
	}
}

/****************************************************/
/**
 * Get the libfabric MR description.
 * @param ptr Base address of the segment.
 * @param size Size of the segment.
 * @return The memory region descriptor or null if not found.
**/
fid_mr* LibfabricDomain::getFidMR ( void* ptr, size_t size )
{
	//checks
	assert(ptr != NULL);
	assert(size > 0);

	//get
	MemoryRegion * region = getMR(ptr,size);
	if (region == nullptr)
		return nullptr;
	else
		return region->mr;
}

/****************************************************/
/**
 * Get the memory region description.
 * @param ptr Base address of the segment.
 * @param size Size of the segment.
 * @return Pointer to the memory region object or null if not found.
**/
MemoryRegion* LibfabricDomain::getMR ( void* ptr, size_t size )
{
	//checks
	assert(ptr != NULL);
	assert(size > 0);

	//search
	MemoryRegion * mr = nullptr;

	//CRITICAL SECTION
	{
		//take lock
		std::lock_guard<std::mutex> guard(this->segmentMutex);
	
		//quick search
		auto it = segments.lower_bound(ptr);

		//if found
		if (it != segments.end()) {
			if (it->second.ptr <= ptr && (char*)it->second.ptr + it->second.size > ptr) {
				if ((char*)ptr+size > (char*)it->second.ptr + it->second.size) {
					DAQ_WARNING_ARG("Caution, a segment from libfabric not completetly fit with the request which is larger : wanted: %1:%2:%3, found: %4:%5:%6")
						.arg(ptr).arg(size).arg((void*)((char*)ptr+size))
						.arg(it->second.ptr).arg(it->second.size).arg((void*)((char*)it->second.ptr+it->second.size))
						.end();
				}
				mr = &(it->second);
			}
		}
	}

	//return 
	return mr;
}

/****************************************************/
/**
 * We might want to use pre-registered buffers for sendMessage operations. This function
 * set their size to prepare them and keep track of them.
 * @param size Size of the segments we want to be allocated.
**/
void LibfabricDomain::setMsgBufferSize(size_t size)
{
	assume(this->msgBuffers.empty(), "Try to change the message buffer size while we already allocated buffers in the pool !");
	this->msgBufferSize = size;
}

/****************************************************/
/**
 * Get a pre-registerd buffer for sendMessage operations. (This do not seems required, but in case).
 * It has a fixed size which is defined by setMsgBuffeSize().
 * @return Return the requested segment.
**/
void * LibfabricDomain::getMsgBuffer(void)
{
	//vars
	void * res = NULL;

	//CRITICAL SECTION
	{
		//take lock
		std::lock_guard<std::mutex> guard(this->msgBuffersMutex);

		//check if need to allocate one
		if (this->msgBuffers.empty()) {
			assume(this->msgBufferSize != 0, "You need to configure message buffer size of libfabric domain !");
			res = malloc(this->msgBufferSize);
			this->registerSegment(res, this->msgBufferSize, true, false, false);
		} else {
			res = this->msgBuffers.back();
			this->msgBuffers.pop_back();
		}
	}

	//ret
	return res;
}

/****************************************************/
/**
 * Return a message buffer which has been used to sendMessage() operation so it can
 * be reused latter.
 * @param buffer The buffer to return.
**/
void LibfabricDomain::retMsgBuffer(void * buffer)
{
	//check
	assume(buffer != NULL, "Fail to return NULL buffer !");

	//CRITICAL SECTION
	{
		//take lock
		std::lock_guard<std::mutex> guard(this->msgBuffersMutex);

		//append
		this->msgBuffers.push_back(buffer);
	}
}

/****************************************************/
/**
 * Return the size of the message buffer being allocated.
**/
size_t LibfabricDomain::getMsgBufferSize(void)
{
	return this->msgBufferSize;
}

}
