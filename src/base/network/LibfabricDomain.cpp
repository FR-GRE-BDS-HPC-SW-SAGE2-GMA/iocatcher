/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cassert>
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
	hints->caps = FI_MSG; //|FI_RMA;
	//hints->caps = FI_MSG;
	//hints->mode = FI_CONTEXT | FI_RX_CQ_DATA;
	//hints->mode = FI_LOCAL_MR;
	hints->ep_attr->type = FI_EP_RDM;
	//hints->fabric_attr->prov_name = strdup("verbs;ofi_rxm");

	//get fi_info
	int err;
	if (isDomainServer)
		err = fi_getinfo(FI_VERSION(1,11), serverIp.c_str(), port.c_str(), FI_SOURCE, hints, &this->fi);
	else
		err = fi_getinfo(FI_VERSION(1,11), serverIp.c_str(), port.c_str(), 0, hints, &this->fi);
	LIBFABRIC_CHECK_STATUS("fi_getinfo",err);
	printf("%d\n", this->fi->caps & FI_MSG);
	printf("%d\n", this->fi->caps & FI_RMA);

	//display
	IOC_INFO_ARG("NET: %1").arg(fi->fabric_attr->prov_name).end();

	//IOC_
	err = fi_fabric(fi->fabric_attr, &this->fabric, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_fabric",err);

	//setup domain
	err = fi_domain(fabric, fi, &this->domain, nullptr);
	LIBFABRIC_CHECK_STATUS("fi_domain",err);
}

/****************************************************/
LibfabricDomain::~LibfabricDomain(void)
{
	fi_close(&domain->fid);
	fi_close(&fabric->fid);
	fi_freeinfo(fi);
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
Iov LibfabricDomain::registerSegment(void * ptr, size_t size)
{
	MemoryRegion region;
	region.ptr = ptr;
	region.size = size;
	region.mr = nullptr;

	//reg into OFI
	int ret = fi_mr_reg(domain, ptr, size, FI_REMOTE_READ | FI_REMOTE_WRITE, 0, 0/*TOTO*/, 0, &(region.mr), nullptr);
	LIBFABRIC_CHECK_STATUS("fi_mr_reg",ret);

	segments.push_back(region);

	Iov iov;
	iov.addr = ptr;
	iov.key = fi_mr_key(region.mr);

	return iov;
}

fid_mr* LibfabricDomain::getFidMR ( void* ptr, size_t size )
{
	MemoryRegion * region = getMR(ptr,size);
	if (region == nullptr)
		return nullptr;
	else
		return region->mr;
}

MemoryRegion* LibfabricDomain::getMR ( void* ptr, size_t size )
{
	//search
	MemoryRegion * mr = nullptr;
	for (auto it = segments.begin() ; it != segments.end() ; ++it) {
		if (it->ptr <= ptr && (char*)it->ptr + it->size > ptr) {
			if ((char*)ptr+size > (char*)it->ptr + it->size)
				DAQ_WARNING("Caution, a segment from libfabric not completetly fit with the request which is larger.");
			mr = &(*it);
			break;
		}
	}
	return mr;
}

}
