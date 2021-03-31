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
#include "MemoryBackendNvdimm.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
MemoryBackendNvdimm::MemoryBackendNvdimm(LibfabricDomain * lfDomain, const std::string & directory)
	:MemoryBackend(lfDomain)
{
	//setup
	this->directory = directory;

	//calc filename
	std::string ftemplate = directory + std::string("/") + "iocatcher-nvdimm-file-XXXXXX";
	char * fname = new char[ftemplate.size()+1];
	memcpy(fname, ftemplate.c_str(), ftemplate.size()+1);

	//open file
	this->fileFD = mkstemp(fname);
	assumeArg(this->fileFD > 0, "Fail to create and open the nvdimm file '%1': %2").arg(fname).argStrErrno().end();

	//unlink so it will be deleted at exit
	int status = unlink(fname);
	assumeArg(status == 0, "Fail to unlink file %1: %2").arg(fname).argStrErrno().end();

	//free mem
	delete [] fname;

	//setup default
	this->fileSize = 0;
	this->chunks = 0;
}

/****************************************************/
MemoryBackendNvdimm::~MemoryBackendNvdimm(void)
{
	//check
	assert(chunks == 0);

	//close the file
	close(this->fileFD);
}

/****************************************************/
void * MemoryBackendNvdimm::allocate(size_t size)
{
	//check
	assert(size > 0);
	assert(size % 4096 == 0);

	//get offset
	size_t offset = this->fileSize;

	//extend the file
	this->fileSize += size;
	int status = ftruncate(this->fileFD, this->fileSize);
	assumeArg(status == 0, "Failed to ftruncate the nvdimm file to size %1").arg(this->fileSize).end();
	
	//memory map
	void * ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, this->fileFD, offset);
	assumeArg(ptr != MAP_FAILED, "Failed to memory map the nvdimm file new range : %1").argStrErrno().end();

	//register for RDMA
	if (this->lfDomain != NULL)
		this->lfDomain->registerSegment(ptr, size, true, true, true);

	//inct
	this->chunks++;

	//return 
	return ptr;
}

/****************************************************/
void MemoryBackendNvdimm::deallocate(void * addr, size_t size)
{
	//check
	assert(addr != NULL);
	assert(size > 0);
	assert(size % 4096 == 0);
	assert(chunks > 0);

	//deregister
	if (this->lfDomain != NULL)
		this->lfDomain->unregisterSegment(addr, size);

	//unmap
	munmap(addr, size);

	//decr
	this->chunks--;
}

/****************************************************/
size_t MemoryBackendNvdimm::getFileSize(void) const
{
	return this->fileSize;
}
