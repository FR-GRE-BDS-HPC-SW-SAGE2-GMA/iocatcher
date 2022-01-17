/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_OBJECT_HPP
#define IOC_OBJECT_HPP

/****************************************************/
//std
#include <cstdint>
#include <cstdlib>
#include <list>
#include <map>
#include <ostream>
#include <vector>
#include <string>
//linux
#include <sys/uio.h>
//internal
#include "ObjectSegment.hpp"
#include "MemoryBackend.hpp"
#include "StorageBackend.hpp"
#include "ConsistencyTracker.hpp"
#include "../../base/network/LibfabricDomain.hpp"
#include "../../base/network/Protocol.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Define the access mode for getBuffers() operation to known
 * it we need to trigger COW or not.
**/
enum ObjectAccessMode
{
	/** Declare a read access, do not trigger COW. **/
	ACCESS_READ,
	/** Declare a write access, trigger COW on shared segments. **/
	ACCESS_WRITE,
};

/****************************************************/
/**
 * Define what is object ID.
**/
struct ObjectId
{
	/** Default constructor, do nothing. **/
	ObjectId(void) {};
	/** Constructor to set default values. **/
	ObjectId(int64_t high, int64_t low) {this->high = high; this->low = low;};
	/** Constructor from a network object ID. **/
	ObjectId(const LibfabricObjectId & objectId) {this->high = objectId.high; this->low = objectId.low;};
	/** The low part. **/
	int64_t low;
	/** The high part. **/
	int64_t high;
};

/****************************************************/
/** Define an object segment list. **/
typedef std::list<ObjectSegmentDescr> ObjectSegmentList;
/** Define an object segment map identified by its offset. **/
typedef std::map<size_t, ObjectSegment> ObjectSegmentMap;

/****************************************************/
class Object
{
	public:
		Object(StorageBackend * backend, MemoryBackend * memBackend, const ObjectId & objectId, size_t alignement = 0);
		const ObjectId & getObjectId(void);
		char * getUniqBuffer(size_t base, size_t size, ObjectAccessMode accessMode, bool load = true);
		bool getBuffers(ObjectSegmentList & segments, size_t base, size_t size, ObjectAccessMode accessMode, bool load = true, bool isForWriteOp = false);
		void fillBuffer(size_t offset, size_t size, char value);
		bool checkBuffer(size_t offset, size_t size, char value);
		bool checkUniq(size_t offset, size_t size);
		static iovec * buildIovec(ObjectSegmentList & segments, size_t offset, size_t size);
		void markDirty(size_t base, size_t size);
		int flush(size_t offset, size_t size);
		int create(void);
		void forceAlignement(size_t alignment);
		ConsistencyTracker & getConsistencyTracker(void);
		Object * makeFullCopyOnWrite(const ObjectId & targetObjectId, bool allowExist);
		void rangeCopyOnWrite(Object & origObject, size_t offset, size_t size);
		void setStorageBackend(StorageBackend * storageBackend);
		void setMemoryBackend(MemoryBackend * memoryBackend);
	private:
		void rangeCopyOnWriteSegment(ObjectSegment & origSegment, size_t offset, size_t size);
		ObjectSegmentDescr loadSegment(size_t offset, size_t size, bool load = true, bool acceptLoadFail = false);
		ssize_t pwrite(void * buffer, size_t size, size_t offset);
		ssize_t pread(void * buffer, size_t size, size_t offset);
		bool isFullyOverlapped(size_t segOffset, size_t segSize, size_t reqOffset, size_t reqSize);
	private:
		/** Object ID **/
		ObjectId objectId;
		/** List of segments hosted by this object. **/
		ObjectSegmentMap segmentMap;
		/** Base alignement to use. **/
		size_t alignement;
		/** Consistency tracker to track ranges mapped by clients and guaranty exclusive write access. **/
		ConsistencyTracker consistencyTracker;
		/** Keep track of the storage backend to be used to dump data. **/
		StorageBackend * storageBackend;
		/** Keep track of the memory backend used to allocate memory. **/
		MemoryBackend * memoryBackend;
};

/****************************************************/
bool operator<(const ObjectSegmentDescr & seg1, const ObjectSegmentDescr & seg2);
bool operator<(const ObjectId & objId1, const ObjectId & objId2);

}

#endif //IOC_OBJECT_HPP
