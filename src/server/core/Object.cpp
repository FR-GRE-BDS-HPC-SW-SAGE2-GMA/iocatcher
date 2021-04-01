/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cassert>
#include <cstring>
#include <iostream>
//linux
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
//internal
#include "Object.hpp"
#include "../../base/common/Debug.hpp"

/****************************************************/
using namespace IOC;
using namespace std;

/****************************************************/
/**
 * Constructor of an object.
 * @param storageBackend Pointer to the storage backend to be used to load/save data.
 * @param objectId The identifier of the object.
 * @param alignement The segment size alignement to be used.
**/
Object::Object(StorageBackend * storageBackend, MemoryBackend * memBack, const ObjectId & objectId, size_t alignement)
{
	this->memoryBackend = memBack;
	this->storageBackend = storageBackend;
	this->alignement = alignement;
	this->objectId = objectId;
}

/****************************************************/
/**
 * @return Return the object ID.
**/
const ObjectId & Object::getObjectId(void)
{
	return this->objectId;
}

/****************************************************/
/**
 * Change the storage backend in use.
 * @param storageBackend The new backend.
**/
void Object::setStorageBackend(StorageBackend * storageBackend)
{
	this->storageBackend = storageBackend;
}

/****************************************************/
/**
 * Change the memory backend in use.
 * @param storageBackend The new backend.
**/
void Object::setMemoryBackend(MemoryBackend * memoryBackend)
{
	this->memoryBackend = memoryBackend;
}

/****************************************************/
/**
 * Mark a given range as dirty.
 * It loops on all segment and mark all overlapping segments as dirty.
 * @todo Can be improved by splitting the segments but need more work
 * to support dirty sub segment tracking.
 * @param base The base offset or the range to mark dirty.
 * @param size Size of the range to mark dirty.
**/
void Object::markDirty(size_t base, size_t size)
{
	//extract
	for (auto & it : this->segmentMap) {
		//if overlap
		if (it.second.overlap(base, size)) {
			it.second.setDirty(true);
		}
	}
}

/****************************************************/
/**
 * Change the force lignement size. This does not be proactive on existing segments.
 * @param alignement.
**/
void Object::forceAlignement(size_t alignment)
{
	assume(this->segmentMap.empty(), "Cannot change the alignement after accessing the object.");
	this->alignement = alignment;
}

/****************************************************/
/**
 * Get the list of object segments matching the given range.
 * @warning Cauton, the first segment can have a lower offset thant the requested offset.
 * It return segments as they are in the object.
 * @param segments The list to fill.
 * @param base The base address of the range to consider.
 * @param size The size of the range to consider.
 * @param accessMode Define the mode of access to know if we need to trigger copy-on-write.
 * @param load If need to load the segment if not present.
**/
void Object::getBuffers(ObjectSegmentList & segments, size_t base, size_t size, ObjectAccessMode accessMode, bool load)
{
	//align
	if (this->alignement > 0)  {
		size += base % alignement;
		base -= base % alignement;
		if (size % alignement > 0)
			size += alignement - (size % alignement);
	}

	//extract
	for (auto it = this->segmentMap.lower_bound(base) ; it != this->segmentMap.end() && it->first < base + size ; ++it) {
		//to ease access
		ObjectSegment & segment = it->second;

		//if overlap
		if (segment.overlap(base, size)) {
			//check if need to cow
			if (accessMode == ACCESS_WRITE && segment.isCow())
				segment.applyCow();

			//add to list
			segments.push_back(segment.getSegmentDescr());
		}
	}

	//complete
	size_t lastOffset = base;
	ObjectSegmentList tmp;
	for (auto it : segments) {
		if (it.offset > lastOffset) {
			size_t size = it.offset - lastOffset;
			tmp.push_back(this->loadSegment(lastOffset, size, load));
		}
		lastOffset = it.offset + it.size;
	}

	//merge list
	for (auto it : tmp)
		segments.push_back(it);

	//load last
	size_t endOffset = base + size;
	if (lastOffset < endOffset) {
		size_t size = endOffset - lastOffset;
		segments.push_back(this->loadSegment(lastOffset, size, load));
	}

	//sort
	segments.sort();
}

/****************************************************/
/**
 * Load a segment for the given range. It will allocated its memory (on nvdimm if enabled),
 * then load the data from mero if enabled and register the segment to the libfabric 
 * domain for RDMA operations.
 * @param offset Offset of the range to consider.
 * @param size Size of the range to consider.
 * @param load If need to load data from mero.
 * @return The loaded object segment.
**/
ObjectSegmentDescr Object::loadSegment(size_t offset, size_t size, bool load)
{
	//allocate memory
	char* buffer = (char*)this->memoryBackend->allocate(size);

	//load data
	if (load) {
		size_t status = this->pread(buffer, size, offset);
		assumeArg(status == size, "Fail to helperPread from object (%1) !").arg(status).end();
		if (status != size) {
			status = this->pwrite(buffer, size, offset);
			assumeArg(status == size, "Fail to write to object (%1) !").arg(status).end();
		}
	}

	//register using end address to be able to use lower_bound() to quick search
	ObjectSegment & segment = this->segmentMap[offset+size-1];
	segment = std::move(ObjectSegment(offset, size, buffer, this->memoryBackend));

	//return descr
	return segment.getSegmentDescr();
}

/****************************************************/
/**
 * Loop on all the segments and flush the dirty one overlapping the given range.
 * @param offset Base offset from where to flush.
 * @param size Size of the range to flus. Use 0 to flush all.
**/
int Object::flush(size_t offset, size_t size)
{
	int ret = 0;
	for (auto & it : this->segmentMap) {
		if (it.second.isDirty()) {
			if (size == 0 || it.second.overlap(offset, size)) {
				if (this->pwrite(it.second.getBuffer(), it.second.getSize(), it.second.getOffset()) != (ssize_t)it.second.getSize())
					ret = -1;
				it.second.setDirty(false);
			}
		}
	}
	return ret;
}

/****************************************************/
/**
 * Make a mero object creation before accessing the object.
**/
int Object::create(void)
{
	if (this->storageBackend != NULL)
		return this->storageBackend->create(this->objectId.high, this->objectId.low);
	else
		return 0;
}

/****************************************************/
/** Just a wrapper to check if we have a storage backend or not. **/
ssize_t Object::pwrite(void * buffer, size_t size, size_t offset)
{
	if (this->storageBackend != NULL)
		return this->storageBackend->pwrite(objectId.high, objectId.low, buffer, size, offset);
	else
		return size;
}

/****************************************************/
/** Just a wrapper to check if we have a storage backend or not. **/
ssize_t Object::pread(void * buffer, size_t size, size_t offset)
{
	if (this->storageBackend != NULL)
		return this->storageBackend->pread(objectId.high, objectId.low, buffer, size, offset);
	else
		return size;
}

/****************************************************/
/**
 * Permit to order the ranges to use in std::map
**/
bool IOC::operator< (const ObjectSegmentDescr & seg1, const ObjectSegmentDescr & seg2)
{
	return seg1.offset < seg2.offset;
}

/****************************************************/
/**
 * Order object IDs to be used as inded in std map.
**/
bool IOC::operator<(const ObjectId & objId1, const ObjectId & objId2)
{
	if (objId1.high < objId2.high)
		return true;
	else if (objId1.high == objId2.high)
		return objId1.low < objId2.low;
	else
		return false;
}

/****************************************************/
/**
 * Return the consistency tracker.
**/
ConsistencyTracker & Object::getConsistencyTracker(void)
{
	return this->consistencyTracker;
}

/****************************************************/
/**
 * Build the IOC vector to make scatter/gather RDMA operations.
 * It take a segment list and compute the intersection if not fully matched, 
 * then build the segment list to be used.
 * @param segments The list of object segments to consider.
 * @param offset The base offset of the range to consider.
 * @param size The size of the range to consider.
 * @return An array of iovec struct to be used by libfabric scatter/gather RDMA operations.
 * It need to be freed by the caller with delete.
**/
iovec * Object::buildIovec(ObjectSegmentList & segments, size_t offset, size_t size)
{
	//compute intersection
	for (auto & it : segments) {
		if (it.offset < offset) {
			int delta = offset - it.offset;
			it.ptr += delta;
			it.offset += delta;
			it.size -= delta;
		}
		if (it.offset + it.size > offset + size) {
			int delta = it.offset + it.size - (offset + size);
			it.size -= delta;
		}
	}

	//build iov
	struct iovec * iov = new iovec[segments.size()];
	int cnt = 0;
	for (auto & it : segments) {
		iov[cnt].iov_base = it.ptr;
		iov[cnt].iov_len = it.size;
		cnt++;
	}

	return iov;
}

/****************************************************/
/**
 * Create a copy of the current object in memory and on the remote server with
 * the given new ID.
 * @param targetObjectId The cow object id to create.
 * @param allowExist Do not fail if the object already exist (fail to create)
**/
Object * Object::makeCopyOnWrite(const ObjectId & targetObjectId, bool allowExist)
{
	//spawn the new object
	Object * cow = new Object(storageBackend, memoryBackend, targetObjectId, alignement);

	//Create
	int createStatus = cow->create();
	assume(createStatus == 0 || allowExist, "Failed to create object on the storage for COW !");

	//loop on all segments
	size_t cursor = 0;
	for (auto & it : this->segmentMap) {
		//if need to copy non loaded part make a copy on the sorage
		if (cursor < it.second.getOffset()) {
			size_t copySize = it.second.getOffset() - cursor;
			ssize_t status = storageBackend->makeCowSegment(this->objectId.high, this->objectId.low, targetObjectId.high, targetObjectId.low, cursor, copySize);
			assume(status == (ssize_t)copySize, "Fail to copy on write on the storage for COW !");
		}

		//we make the segment in cow mode
		ObjectSegment & segment = cow->segmentMap[it.first];
		segment.makeCowOf(it.second);

		//if not dirty we flush
		if (segment.isDirty() == false) {
			ssize_t status = cow->pwrite(segment.getBuffer(), segment.getSize(), segment.getOffset());
			assume(status == (ssize_t)segment.getSize(), "Fail to copy the COW non dirty elements !");
		}

		//move
		cursor = it.second.getOffset() + it.second.getSize();
	}

	//ret
	return cow;
}
