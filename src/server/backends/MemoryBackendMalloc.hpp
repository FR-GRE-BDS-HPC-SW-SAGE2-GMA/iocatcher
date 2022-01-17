/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_MEMORY_BACKEND_MALLOC_HPP
#define IOC_MEMORY_BACKEND_MALLOC_HPP

/****************************************************/
//internal
#include "../core/MemoryBackend.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement basic memory allocation via malloc, just
 * automatically register it to the RDMA domain for RDMA
 * operations.
**/
class MemoryBackendMalloc: public MemoryBackend
{
	public:
		MemoryBackendMalloc(LibfabricDomain * lfDomain) : MemoryBackend(lfDomain) {};
	public:
		virtual void * allocate(size_t size);
		virtual void deallocate(void * addr, size_t size);
};

}

#endif //IOC_MEMORY_BACKEND_MALLOC_HPP
