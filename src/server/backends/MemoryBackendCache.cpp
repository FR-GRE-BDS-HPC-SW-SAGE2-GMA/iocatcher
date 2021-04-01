/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <cassert>
//unix
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
//internal
#include "base/common/Debug.hpp"
#include "MemoryBackendCache.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the cached memory backend.
 * @param backend Pointer to the sub backend to 
 * use to allocate and free memory.
**/
MemoryBackendCache::MemoryBackendCache(MemoryBackend * backend)
	:MemoryBackend(NULL)
{
	//check
	assert(backend != NULL);

	//setup
	this->backend = backend;
}

/****************************************************/
/**
 * Destructor of the memory backend, it free all the allocated memory.
 * memory.
**/
MemoryBackendCache::~MemoryBackendCache(void)
{
	//check that all lists are free
	size_t cnt = 0;
	for (auto & it : this->freeLists)
		cnt += it.second.size();
	if (cnt != this->rangesTracker.size())
		IOC_WARNING_ARG("Missing elements in free list, might not have free all: %1 != %2")
			.arg(cnt)
			.arg(this->rangesTracker.size())
			.end();
	assert(cnt == this->rangesTracker.size());

	//remove ranges
	for (auto & it : this->rangesTracker) {
		this->backend->deallocate(it.first, it.second);
	}

	//clear
	this->rangesTracker.clear();
	this->freeLists.clear();

	//delete backend
	delete this->backend;
}

/****************************************************/
/**
 * Check if chunks are available in the cache and return it,
 * if not found allocate new memory to the sub backend.
 * @param size Size of the desired memory.
**/
void * MemoryBackendCache::allocate(size_t size)
{
	//check
	assert(size > 0);

	//search in preexisting
	auto & freeList = this->freeLists[size];

	//check if empty
	void * ptr = NULL;
	if (freeList.empty()) {
		//get from backend
		ptr = this->backend->allocate(size);

		//register to range tracker
		this->rangesTracker[ptr] = size;
	} else {
		//extract first from list
		ptr = freeList.front();
		freeList.pop_front();
	}

	//return 
	return ptr;
}

/****************************************************/
/**
 * Return the given chunk to the cache and register it
 * for latter use. Notice, we currently free the memory
 * only at exit.
 * @param addr Address of the memory to return.
 * @param size Size of the memory to return.
**/
void MemoryBackendCache::deallocate(void * addr, size_t size)
{
	//check
	assert(addr != NULL);
	assert(size > 0);

	//check
	assert(isLocalMemory(addr, size));

	//register to free list
	this->freeLists[size].push_front(addr);
}

/****************************************************/
/**
 * For debugging check if the given pointer belongs to the cache.
 * @param ptr Address of the memory to return.
 * @param size Size of the memory to return.
**/
bool MemoryBackendCache::isLocalMemory(void * ptr, size_t size)
{
	//check
	assert(ptr != NULL);
	assert(size > 0);
	assert(size % 4096 == 0);

	//search
	auto it = this->rangesTracker.find(ptr);

	//return
	if (it == this->rangesTracker.end()) {
		return false;
	} else {
		assert(it->second == size);
		return true;
	}
}
