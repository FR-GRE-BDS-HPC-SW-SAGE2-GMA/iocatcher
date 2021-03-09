/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_STORAGE_BACKEND_MERO_HPP
#define IOC_STORAGE_BACKEND_MERO_HPP

/****************************************************/
//std
#include "../core/StorageBackend.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement a storage backend for the Mero object storage.
**/
class StorageBackendMero : public StorageBackend
{
	public:
		StorageBackendMero(void);
		~StorageBackendMero(void);
		virtual ssize_t pread(int64_t high, int64_t low, void * buffer, size_t size, size_t offset) override;
		virtual ssize_t pwrite(int64_t high, int64_t low, void * buffer, size_t size, size_t offset) override;
		virtual int create(int64_t high, int64_t low) override;
};

}

#endif //IOC_STORAGE_BACKEND_MERO_HPP
