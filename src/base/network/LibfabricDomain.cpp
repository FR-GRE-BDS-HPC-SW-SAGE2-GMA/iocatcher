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

/****************************************************/
namespace IOC
{

/****************************************************/
LibfabricDomain::LibfabricDomain(std::string & serverIp, std::string & port, bool isDomainServer)
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
	hints->caps = FI_MSG;
	//hints->caps = FI_MSG;
	//hints->mode = FI_CONTEXT | FI_RX_CQ_DATA;
	//hints->mode = FI_LOCAL_MR;
	hints->ep_attr->type = FI_EP_RDM;

	//get fi_info
	int err;
	if (isDomainServer)
		err = fi_getinfo(FI_VERSION(1,11), serverIp.c_str(), port.c_str(), FI_SOURCE, hints, &this->fi);
	else
		err = fi_getinfo(FI_VERSION(1,11), serverIp.c_str(), port.c_str(), 0, hints, &this->fi);
	LIBFABRIC_CHECK_STATUS("fi_getinfo",err);

	//display
	IOC_INFO_ARG("NEt: %1").arg(fi->fabric_attr->prov_name).end();

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

}
