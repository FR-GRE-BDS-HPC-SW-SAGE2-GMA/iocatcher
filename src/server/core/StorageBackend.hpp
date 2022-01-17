/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_STORAGE_BACKEND_HPP
#define IOC_STORAGE_BACKEND_HPP

/****************************************************/
//std
#include <cstdlib>

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * A storage backend is an object handling the read and write operation to
 * data storage from the object in order to read / write or flush the data
 * to/from the storage.
**/
class StorageBackend
{
	public:
		/** Default constructor of a storage backend. **/
		StorageBackend(void);
		/** Default virtual destructor of a storage backend to enable inheritance. **/
		virtual ~StorageBackend(void);
		/**
		 * Helper function to make a read operation on a mero object.
		 * @param high The high part of the object ID.
		 * @param low The low part of the object ID.
		 * @param buffer The data to write.
		 * @param size Size of the data to read.
		 * @param offset Offset in the mero object.
		 * @return The size which has been read or negativ number in case of error.
		**/
		virtual ssize_t pread(int64_t high, int64_t low, void * buffer, size_t size, size_t offset) = 0;
		/**
		 * Helper function to make a write operation on a mero object.
		 * @param high The high part of the object ID.
		 * @param low The low part of the object ID.
		 * @param buffer The data to write.
		 * @param size Size of the data to write.
		 * @param offset Offset in the mero object.
		 * @return The size which has been written or negativ number in case of error.
		**/
		virtual ssize_t pwrite(int64_t high, int64_t low, void * buffer, size_t size, size_t offset) = 0;
		/**
		 * Make a mero object creation before accessing the object.
		**/
		virtual int create(int64_t high, int64_t low) = 0;
		virtual ssize_t makeCowSegment(int64_t highOrig, int64_t lowOrig, int64_t highDest, int64_t lowDest, size_t offset, size_t size);
};

}

#endif //IOC_STORAGE_BACKEND_HPP
