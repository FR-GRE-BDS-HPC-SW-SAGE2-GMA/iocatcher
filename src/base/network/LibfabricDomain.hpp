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
#include <list>
//libfabric
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_domain.h>

//TMP
//#define TEST_RDMA_SIZE (4*1024*1024)
#define TEST_RDMA_SIZE (4096)

/****************************************************/
namespace IOC
{

/****************************************************/
struct MemoryRegion
{
	void * ptr;
	size_t size;
	fid_mr * mr;
};

/****************************************************/
struct Iov
{
	void * addr;
	uint64_t key;
};

/****************************************************/
class LibfabricDomain
{
	public:
		LibfabricDomain(const std::string & server, const std::string & port, bool isDomainServer);
		~LibfabricDomain(void);
		Iov registerSegment(void * ptr, size_t size);
		void unregisterSegment(void * ptr, size_t size);
		fi_info * getFiInfo(void);
		fid_fabric * getFabric(void);
		fid_domain * getDomain(void);
		fid_mr* getFidMR ( void* ptr, size_t size );
		MemoryRegion* getMR ( void* ptr, size_t size );
	private:
		fi_info *fi;
		fid_fabric *fabric;
		fid_domain *domain;
		std::list<MemoryRegion> segments;
};

}

#endif //IOC_LIBFABRIC_DOMAIN_HPP
