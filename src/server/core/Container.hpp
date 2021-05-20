/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_CONTAINER_HPP
#define IOC_CONTAINER_HPP

/****************************************************/
#include <cstdint>
#include <cstdlib>
#include <map>
#include "Object.hpp"
#include "MemoryBackend.hpp"
#include "StorageBackend.hpp"
#include "../../base/network/LibfabricDomain.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * A container aggregate all the handle objects and provide the necessary
 * function to find them from object IDs.
**/
class Container
{
	public:
		Container(StorageBackend * storageBackend, MemoryBackend * memBack, size_t objectSegmentsAlignement = 0);
		~Container(void);
		Object & getObject(const ObjectId & objectId);
		bool hasObject(const ObjectId & objectId);
		bool makeObjectFullCow(const ObjectId & sourceId, const ObjectId &destId, bool allowExist);
		void onClientDisconnect(uint64_t clientId);
		void setObjectSegmentsAlignement(size_t alignement);
		void setStorageBackend(StorageBackend * storageBackend);
		void setMemoryBackend(MemoryBackend * memoryBackend);
	private:
		/** List ob objects identified by their object ID. **/
		std::map<ObjectId, Object*> objects;
		/** We can force a minimal size for the object segments to get better performance. **/
		size_t objectSegmentsAlignement;
		/** Keep track of the storage backend to use. **/
		StorageBackend * storageBackend;
		/** Keep track of the memory backend in use. **/
		MemoryBackend * memoryBackend;
};

}

#endif //IOC_CONTAINER_HPP
