/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_LIBFABRIC_DOMAIN_HPP
#define IOC_LIBFABRIC_DOMAIN_HPP

/****************************************************/
//std
#include <string>
//libfabric
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_domain.h>

/****************************************************/
namespace IOC
{

/****************************************************/
class LibfabricDomain
{
	public:
		LibfabricDomain(const std::string & server, const std::string & port, bool isDomainServer);
		~LibfabricDomain(void);
		fi_info * getFiInfo(void);
		fid_fabric * getFabric(void);
		fid_domain * getDomain(void);
	private:
		fi_info *fi;
		fid_fabric *fabric;
		fid_domain *domain;
};

}

#endif //IOC_LIBFABRIC_DOMAIN_HPP
