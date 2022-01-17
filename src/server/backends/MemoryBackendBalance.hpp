/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_MEMORY_BACKEND_ROUND_ROBIN_HPP
#define IOC_MEMORY_BACKEND_ROUND_ROBIN_HPP

/****************************************************/
//std
#include <vector>
#include <map>
//internal
#include "../core/MemoryBackend.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement a round robin memory backend permitting
 * to distribute the allocations over several backends
 * by keeping a balance over the memory size allocated
 * on each one.
**/
class MemoryBackendBalance: public MemoryBackend
{
	public:
		MemoryBackendBalance(void);
		virtual ~MemoryBackendBalance(void);
		void registerBackend(MemoryBackend * backend);
		size_t getMem(int id) const;
	public:
		virtual void * allocate(size_t size);
		virtual void deallocate(void * addr, size_t size);
	private:
		/** Keep track of all the sub backends. **/
		std::vector<MemoryBackend *> backends;
		/** Keep track of the memory space used on each backend. **/
		std::vector<size_t> backendMem;
		/** Keep track to which backend the address belong. **/
		std::map<void*, int> backendOfMem;
};

}

#endif //IOC_MEMORY_BACKEND_ROUND_ROBIN_HPP
