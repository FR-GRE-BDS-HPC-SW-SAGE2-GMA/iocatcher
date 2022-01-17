/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_MEMORY_BACKEND_NVDIMM_GROW_HPP
#define IOC_MEMORY_BACKEND_NVDIMM_GROW_HPP

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
 * Implement memory backend using nvdimm memory. This class
 * implement only the file allocation strategy which only grow
 * the nvdimm file, it requires to be embedded into a 
 * MemoryBackendCache to be efficient.
 * This version of the implementation use ftunctate to grow
 * the file for every new allocation.
 *
 * LIMITATION: There is currently a bug in the kernel which make
 * ftruncate() blocked when the file mapped and pin memory
 * for rdma communications.
**/
class MemoryBackendNvdimmGrow: public MemoryBackend
{
	public:
		MemoryBackendNvdimmGrow(LibfabricDomain * lfDomain, const std::string & directory);
		virtual ~MemoryBackendNvdimmGrow(void);
		size_t getFileSize(void) const;
	public:
		virtual void * allocate(size_t size);
		virtual void deallocate(void * addr, size_t size);
	private:
		/** directory in which to store the nvdimm data. **/
		std::string directory;
		/** The file descriptor handling the nvdimm file. **/
		int fileFD;
		/** Current size of the file. **/
		size_t fileSize;
		/** Count allocated chuncks **/
		size_t chunks;
};

}

#endif //IOC_MEMORY_BACKEND_NVDIMM_GROW_HPP
