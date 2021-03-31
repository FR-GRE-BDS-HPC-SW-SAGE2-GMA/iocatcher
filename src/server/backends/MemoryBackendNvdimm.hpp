/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_MEMORY_BACKEND_NVDIMM_HPP
#define IOC_MEMORY_BACKEND_NVDIMM_HPP

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
 * Implement memory backend using nvdimm memory. The class
 * support batch allocation and group allocation in a limiited
 * number of files to avoid sqaushing the fd limit.
**/
class MemoryBackendNvdimm: public MemoryBackend
{
	public:
		MemoryBackendNvdimm(LibfabricDomain * lfDomain, const std::string & directory);
		virtual ~MemoryBackendNvdimm(void);
	public:
		virtual void * allocate(size_t size);
		virtual void deallocate(void * addr, size_t size);
	private:
		bool isLocalMemory(void * ptr, size_t size);
	private:
		/** directory in which to store the nvdimm data. **/
		std::string directory;
		/** The file descriptor handling the nvdimm file. **/
		int fileFD;
		/** Current size of the file. **/
		size_t fileSize;
		/** Keep track of the free memory. **/
		std::map<size_t, std::list<void*>> freeLists;
		/** register the allocated ranges. **/
		std::map<void*, size_t> rangesTracker;
};

}

#endif //IOC_MEMORY_BACKEND_NVDIMM_HPP
