/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_MEMORY_BACKEND_HPP
#define IOC_MEMORY_BACKEND_HPP

/****************************************************/
//std
#include <cstdlib>
//internal
#include "Object.hpp"
#include "base/network/LibfabricDomain.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * A memory backend handle the memory allocations of memory segments.
 * There can be two implementation, a malloc base one and one for
 * nvdimms.
**/
class MemoryBackend
{
	public:
		MemoryBackend(LibfabricDomain * lfDomain);
		virtual ~MemoryBackend(void);
		virtual void * allocate(size_t size) = 0;
		virtual void deallocate(void * addr, size_t size) = 0;
		LibfabricDomain * getLfDomain(void);
	protected:
		/** Keep track of the libfabric domain for memory registration/deregistration. **/
		LibfabricDomain * lfDomain;
};

}

#endif //IOC_MEMORY_BACKEND_HPP
