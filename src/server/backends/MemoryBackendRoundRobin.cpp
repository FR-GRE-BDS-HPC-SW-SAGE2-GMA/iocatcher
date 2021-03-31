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
#include "MemoryBackendRoundRobin.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
MemoryBackendRoundRobin::MemoryBackendRoundRobin(void)
	:MemoryBackend(NULL)
{
}

/****************************************************/
MemoryBackendRoundRobin::~MemoryBackendRoundRobin(void)
{
	//warn if not all zero
	if (this->backendOfMem.empty() == false)
		IOC_WARNING_ARG("Delete backends but one have memory : %1 !").arg(this->backendOfMem.size()).end();

	//delete all backends
	for (auto & it : this->backends)
		delete it;
}

/****************************************************/
void MemoryBackendRoundRobin::registerBackend(MemoryBackend * backend)
{
	this->backends.push_back(backend);
	this->backendMem.push_back(0);
}

/****************************************************/
void * MemoryBackendRoundRobin::allocate(size_t size)
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
void MemoryBackendRoundRobin::deallocate(void * addr, size_t size)
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
size_t MemoryBackendRoundRobin::getMem(int id) const
{
	//check
	assert(id >= 0);
	assert(id < this->backendMem.size());

	//return
	return this->backendMem[id];
}