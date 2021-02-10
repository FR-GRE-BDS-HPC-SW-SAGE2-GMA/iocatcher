/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_OBJECT_HPP
#define IOC_OBJECT_HPP

/****************************************************/
#include <cstdint>
#include <cstdlib>
#include <list>
#include <map>
#include <ostream>
#include "ConsistencyTracker.hpp"
#include "../../base/network/LibfabricDomain.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
struct ObjectId
{
	int64_t low;
	int64_t high;
};

/****************************************************/
struct ObjectSegment
{
	char * ptr;
	size_t offset;
	size_t size;
	bool dirty;
	bool overlap(size_t segBase, size_t segSize);
};

/****************************************************/
typedef std::list<ObjectSegment> ObjectSegmentList;
typedef std::map<size_t, ObjectSegment> ObjectSegmentMap;

/****************************************************/
class Object
{
	public:
		Object(LibfabricDomain * domain, int64_t low, int64_t high, size_t alignement = 0);
		const ObjectId & getObjectId(void);
		void getBuffers(ObjectSegmentList & segments, size_t base, size_t size, bool load = true);
		void markDirty(size_t base, size_t size);
		int flush(size_t offset, size_t size);
		int create(void);
		void forceAlignement(size_t alignment);
		static void setNvdimm(const char * path);
		ConsistencyTracker & getConsistencyTracker(void);
	private:
		ObjectSegment loadSegment(size_t offset, size_t size, bool load = true);
	private:
		LibfabricDomain * domain;
		ObjectId objectId;
		ObjectSegmentMap segmentMap;
		static const char * nvdimmPath;
		size_t alignement;
		ConsistencyTracker consistencyTracker;
};

/****************************************************/
bool operator<(const ObjectSegment & seg1, const ObjectSegment & seg2);
bool operator<(const ObjectId & objId1, const ObjectId & objId2);

}

#endif //IOC_OBJECT_HPP
