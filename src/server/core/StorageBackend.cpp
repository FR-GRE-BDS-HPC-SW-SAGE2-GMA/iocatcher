/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
//std
#include "../../base/common/Debug.hpp"
#include "StorageBackend.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
StorageBackend::StorageBackend(void)
{
}

/****************************************************/
StorageBackend::~StorageBackend(void)
{
}

/****************************************************/
ssize_t StorageBackend::makeCowSegment(int64_t highOrig, int64_t lowOrig, int64_t highDest, int64_t lowDest, size_t offset, size_t size)
{
	//debug
	IOC_DEBUG_ARG("storage-backend", "Apply manual COW on segment %1:%2 -> %3:%4 [%5,%6]")
		.arg(highOrig)
		.arg(lowOrig)
		.arg(highDest)
		.arg(lowDest)
		.arg(offset)
		.arg(size)
		.end();

	//allocate tmp buffer
	char * buffer = new char[size];

	//load
	ssize_t status = this->pread(highOrig, lowOrig, buffer, size, offset);
	assumeArg(status == (ssize_t)size, "Fail to fully load the object range for COW : %1:%2 -> %3:%4 [%5,%6] !")
		.arg(highOrig)
		.arg(lowOrig)
		.arg(highDest)
		.arg(lowDest)
		.arg(offset)
		.arg(size)
		.end();
	
	//write
	status = this->pwrite(highDest, lowDest, buffer, size, offset);
	assumeArg(status == (ssize_t)size, "Fail to fully write back the object range for COW : %1:%2 -> %3:%4 [%5,%6] !")
		.arg(highOrig)
		.arg(lowOrig)
		.arg(highDest)
		.arg(lowDest)
		.arg(offset)
		.arg(size)
		.end();
	
	//free mem
	delete [] buffer;

	//retu
	return size;
}
