/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cassert>
//internal
#include "base/common/Debug.hpp"
#include "MemoryBackendMalloc.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Allocate new memory chunk using malloc and register it to the
 * libfabric domain for RDMA operations.
 * @param size Size of the memory to allocate.
**/
void * MemoryBackendMalloc::allocate(size_t size)
{
	//check
	assert(size > 0);

	//allocate
	void * ptr = malloc(size);
	assume(ptr != NULL, "Fail to allocate memory, might get out of memory !");

	//register
	if (this->lfDomain != NULL)
		this->lfDomain->registerSegment(ptr, size, true, true, false);
	
	//return
	return ptr;
}

/****************************************************/
/**
 * Free the given memory pointer.
 * @param addr Address of the memory space to free.
 * @param size Size of the memory space used to unregister it
 * from the libfabric domain.
**/
void MemoryBackendMalloc::deallocate(void * addr, size_t size)
{
	//check
	assert(addr != NULL);
	assert(size > 0);

	//de-register
	if (this->lfDomain != NULL)
		this->lfDomain->unregisterSegment(addr, size);

	//free
	free(addr);
}
