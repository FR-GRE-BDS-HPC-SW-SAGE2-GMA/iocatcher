/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
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
#include "StorageBackend.hpp"
#include "ConsistencyTracker.hpp"
#include "../../base/network/LibfabricDomain.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Define what is object ID.
**/
struct ObjectId
{
	/** The low part. **/
	int64_t low;
	/** The high part. **/
	int64_t high;
};

/****************************************************/
/**
 * Define an object segment. It match with what has been requested by clients
 * via read/write operations.
**/
struct ObjectSegment
{
	//functions
	bool overlap(size_t segBase, size_t segSize);

	//members
	/** Address of the memory buffer storing this segment. **/
	char * ptr;
	/** Offset of this segment. **/
	size_t offset;
	/** Size of this segment. **/
	size_t size;
	/** Store ditry state to know if we need to flush it or not. **/
	bool dirty;
};

/****************************************************/
/** Define an object segment list. **/
typedef std::list<ObjectSegment> ObjectSegmentList;
/** Define an object segment map identified by its offset. **/
typedef std::map<size_t, ObjectSegment> ObjectSegmentMap;

/****************************************************/
class Object
{
	public:
		Object(StorageBackend * backend, LibfabricDomain * domain, int64_t low, int64_t high, size_t alignement = 0);
		const ObjectId & getObjectId(void);
		void getBuffers(ObjectSegmentList & segments, size_t base, size_t size, bool load = true);
		static iovec * buildIovec(ObjectSegmentList & segments, size_t offset, size_t size);
		void markDirty(size_t base, size_t size);
		int flush(size_t offset, size_t size);
		int create(void);
		void forceAlignement(size_t alignment);
		static void setNvdimm(const std::vector<std::string> & paths);
		ConsistencyTracker & getConsistencyTracker(void);
	private:
		ObjectSegment loadSegment(size_t offset, size_t size, bool load = true);
		ssize_t pwrite(int64_t high, int64_t low, void * buffer, size_t size, size_t offset);
		ssize_t pread(int64_t high, int64_t low, void * buffer, size_t size, size_t offset);
	private:
		/** Libfabric domain to be used for meomry registration. **/
		LibfabricDomain * domain;
		/** Object ID **/
		ObjectId objectId;
		/** List of segments hosted by this object. **/
		ObjectSegmentMap segmentMap;
		/** 
		 * Paths to nvdim to allocate memory on it.
		 * @todo Make cleaner code avoiding this static.
		**/
		static std::vector<std::string> nvdimmPaths;
		/** Base alignement to use. **/
		size_t alignement;
		/** Consistency tracker to track ranges mapped by clients and guaranty exclusive write access. **/
		ConsistencyTracker consistencyTracker;
		/** ID of file creation on the NVDIMM. **/
		int nvdimmId;
		/** Keep track of the storage backend to be used to dump data. **/
		StorageBackend * storageBackend;
};

/****************************************************/
bool operator<(const ObjectSegment & seg1, const ObjectSegment & seg2);
bool operator<(const ObjectId & objId1, const ObjectId & objId2);

}

#endif //IOC_OBJECT_HPP
