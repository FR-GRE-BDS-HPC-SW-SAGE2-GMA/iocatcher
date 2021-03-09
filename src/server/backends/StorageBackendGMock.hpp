/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_STORAGE_BACKEND_GMOCK_HPP
#define IOC_STORAGE_BACKEND_GMOCK_HPP

/****************************************************/
//std
#include <gmock/gmock.h>
#include "../core/StorageBackend.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement a storage backend for the Mero object storage.
**/
class StorageBackendGMock : public StorageBackend
{
	public:
		MOCK_METHOD(ssize_t, pread, (int64_t high, int64_t low, void * buffer, size_t size, size_t offset), (override));
		MOCK_METHOD(ssize_t, pwrite, (int64_t high, int64_t low, void * buffer, size_t size, size_t offset), (override));
		MOCK_METHOD(int, create, (int64_t high, int64_t low), (override));
};

}

#endif //IOC_STORAGE_BACKEND_GMOCK_HPP
