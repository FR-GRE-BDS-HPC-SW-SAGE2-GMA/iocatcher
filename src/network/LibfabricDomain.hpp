/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_LIBFABRIC_DOMAIN_HPP
#define IOC_LIBFABRIC_DOMAIN_HPP

/****************************************************/
#include <string>
//libfabric
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>

/****************************************************/
namespace IOC
{

/****************************************************/
class LibfabricDomain
{
	public:
		LibfabricDomain(std::string & server, std::string & port, bool isDomainServer);
		~LibfabricDomain(void);
	private:
		struct fi_info *fi;
		struct fid_fabric *fabric;
		struct fid_domain *domain;
};

}

#endif //IOC_LIBFABRIC_DOMAIN_HPP
