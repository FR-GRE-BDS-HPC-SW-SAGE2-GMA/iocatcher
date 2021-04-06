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
#include "MemoryBackendBalance.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the rouond robin memory backend, do nothing.
**/
MemoryBackendBalance::MemoryBackendBalance(void)
	:MemoryBackend(NULL)
{
}

/****************************************************/
/**
 * Destroy all the sub memory backends after checking that all the
 * memory have been freed.
**/
MemoryBackendBalance::~MemoryBackendBalance(void)
{
	//warn if not all zero
	if (this->backendOfMem.empty() == false)
		IOC_WARNING_ARG("Delete backends but one have memory : %1 !").arg(this->backendOfMem.size()).end();

	//delete all backends
	for (auto & it : this->backends)
		delete it;
}

/****************************************************/
/**
 * Register a sub memory backend. It will be destroyed by the round robin
 * backend at exit.
 * @param backend Address of the memory backend to register.
**/
void MemoryBackendBalance::registerBackend(MemoryBackend * backend)
{
	assert(backend != NULL);
	this->backends.push_back(backend);
	this->backendMem.push_back(0);
}

/****************************************************/
/**
 * Allocate a new segment on the less used sub memory backend.
 * @param size Size of the memory segment to allocate.
**/
void * MemoryBackendBalance::allocate(size_t size)
{
	//check if has at least one
	assert(this->backends.size() > 0);

	//set to 0
	size_t id = 0;
	size_t min = this->backendMem[id];

	//search min
	for (int i = 1 ; i < this->backends.size() ; i++) {
		if (this->backendMem[i] < min) {
			id = i;
			min = this->backendMem[i];
		}
	}

	//send request to this one
	void * ptr = this->backends[id]->allocate(size);
	this->backendMem[id] += size;

	//register
	this->backendOfMem[ptr] = id;

	//return 
	return ptr;
}

/****************************************************/
/**
 * Return the given segment to its parent backend.
 * @param addr Address of the memory space to free.
 * @param size Size of the memory space used to free.
**/
void MemoryBackendBalance::deallocate(void * addr, size_t size)
{
	//check
	assert(addr != NULL);
	assert(size > 0);

	//search
	auto it = this->backendOfMem.find(addr);

	//not found
	assumeArg(it != this->backendOfMem.end(), "Fail to bound backend of the given memory : %1 !").arg(addr).end();

	//free
	size_t id = it->second;
	this->backends[id]->deallocate(addr, size);

	//erase & decrement
	this->backendOfMem.erase(it);
	assert(this->backendMem[id] >= size);
	this->backendMem[id] -= size;
}

/****************************************************/
/**
 * Return the memory used by the given memory bacend.
 * Used in unit tests.
 * @param id Id of the memory backend we want to check.
**/
size_t MemoryBackendBalance::getMem(int id) const
{
	//check
	assert(id >= 0);
	assert(id < this->backendMem.size());

	//return
	return this->backendMem[id];
}
