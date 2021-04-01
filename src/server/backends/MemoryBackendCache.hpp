/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_MEMORY_BACKEND_CACHE_HPP
#define IOC_MEMORY_BACKEND_CACHE_HPP

/****************************************************/
//std
#include <string>
#include <map>
//internal
#include "../core/MemoryBackend.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement a cached memory backend to avoid returning the
 * memory. This is required on top of nvdimm backend.
 * The implementation was splitted in two to also be usable
 * with the malloc backend.
**/
class MemoryBackendCache: public MemoryBackend
{
	public:
		MemoryBackendCache(MemoryBackend * backend);
		virtual ~MemoryBackendCache(void);
	public:
		virtual void * allocate(size_t size);
		virtual void deallocate(void * addr, size_t size);
	private:
		bool isLocalMemory(void * ptr, size_t size);
	private:
		/** Keep track of the underhood memory backend to use. **/
		MemoryBackend * backend;
		/** Keep track of the free memory per size class. **/
		std::map<size_t, std::list<void*>> freeLists;
		/** register the allocated ranges. **/
		std::map<void*, size_t> rangesTracker;
};

}

#endif //IOC_MEMORY_BACKEND_CACHE_HPP
