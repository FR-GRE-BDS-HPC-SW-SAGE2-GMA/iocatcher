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
 * Check if the object is fully overlapped by the requested range.
 * @param segOffset The base offset of the segment to be accessed.
 * @param segSize The size of the segment to be accessed.
 * @param reqOffset The base offset of the access request.
 * @param reqSize The size of the access request.
 * @return True if the segment is fully overlaped by the request, false otherwise.
**/
bool Object::isFullyOverlapped(size_t segOffset, size_t segSize, size_t reqOffset, size_t reqSize)
{
	size_t segEnd = segOffset + segSize;
	size_t reqEnd = reqOffset + reqSize;
	return reqOffset >= segOffset && reqEnd <= segEnd;
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
 * @param isForWriteOp For write operation we want to accept pre-read failure and puruse but
 * we want to report error on a read operation. Also on write op we load the data from
 * storage only if the loaded segment is larger than the requested range (in other word
 * if due to alignement the caller will not write the full segment).
 * @return True if OK, false in case it fails to read content while creating the segments.
**/
bool Object::getBuffers(ObjectSegmentList & segments, size_t base, size_t size, ObjectAccessMode accessMode, bool load, bool isForWriteOp)
{
	//keep orig range
	size_t origBase = base;
	size_t origSize = size;

	//align
	if (this->alignement > 0)  {
		size += base % alignement;
		base -= base % alignement;
		if (size % alignement > 0)
			size += alignement - (size % alignement);
	}

	//extract
	for (auto it = this->segmentMap.lower_bound(base) ; it != this->segmentMap.end() && it->second.overlap(base, size) ; ++it) {
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
			bool needLoad = load;
			if (isForWriteOp && isFullyOverlapped(lastOffset, size, origBase, origSize))
				needLoad = false;
			ObjectSegmentDescr descr = this->loadSegment(lastOffset, size, needLoad, isForWriteOp);
			if (descr.ptr == NULL)
				return false;
			tmp.push_back(descr);
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
		bool needLoad = load;
		if (isForWriteOp && isFullyOverlapped(lastOffset, size, origBase, origSize))
			needLoad = false;
		ObjectSegmentDescr descr = this->loadSegment(lastOffset, size, needLoad, isForWriteOp);
		if (descr.ptr == NULL)
			return false;
		segments.push_back(descr);
	}

	//sort
	segments.sort();

	//ok
	return true;
}

/****************************************************/
/**
 * Load a segment for the given range. It will allocated its memory (on nvdimm if enabled),
 * then load the data from mero if enabled and register the segment to the libfabric 
 * domain for RDMA operations.
 * @param offset Offset of the range to consider.
 * @param size Size of the range to consider.
 * @param load If need to load data from mero.
 * @param acceptLoadFail This option is used on a first write access if the write
 * we first load the old data before overriting it. But as it is a write op we
 * do not fail if the load operation fails.
 * @return The loaded object segment.
**/
ObjectSegmentDescr Object::loadSegment(size_t offset, size_t size, bool load, bool acceptLoadFail)
{
	//allocate memory
	char* buffer = (char*)this->memoryBackend->allocate(size);

	//load data
	if (load) {
		size_t status = this->pread(buffer, size, offset);
		//if fail to read
		if (status != size && acceptLoadFail) {
			this->memoryBackend->deallocate(buffer, size);
			ObjectSegmentDescr errDescr = {
				.ptr = NULL,
				.offset = 0,
				.size = 0
			};
			return errDescr;
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
 * Create a copy of the current given segment of a remote object to the current
 * object. If it match an empty region or a local segment with same offset/size
 * it will make it COW.
 * @param origSegment The source object to copy on the current one (this).
 * @param offset Define the offset from which to copy. If it does not be aligned
 * on pre-existing object segments there will be data copies on border segments.
 * @param size Size of the segment to copy.
**/
void Object::rangeCopyOnWriteSegment(ObjectSegment & origSegment, size_t offset, size_t size)
{
	//calculate inner coords
	size_t innerOffset = 0;
	size_t innerSize = origSegment.getSize();
	bool origPartialOverlap = false;

	//calc
	if (origSegment.getOffset() < offset) {
		innerOffset = offset - origSegment.getOffset();
		innerSize -= innerOffset;
		origPartialOverlap = true;
	}
	if (offset + size < origSegment.getOffset() + origSegment.getSize()) {
		innerSize -= (origSegment.getOffset() + origSegment.getSize()) - (offset + size);
		origPartialOverlap = true;
	}

	//search dest and check if same
	//TODO getindex
	const auto & targetSegmentIterator = this->segmentMap.lower_bound(origSegment.getOffset());
	bool localPartialOverlap = false;
	if (targetSegmentIterator != this->segmentMap.end())
		if (targetSegmentIterator->second.getOffset() != origSegment.getOffset() || targetSegmentIterator->second.getSize() != origSegment.getSize())
			localPartialOverlap = true;

	//if partial manual copy
	if (origPartialOverlap || localPartialOverlap) {
		//calc abs
		size_t absOffset = origSegment.getOffset() + innerOffset;

		//get segments
		ObjectSegmentList segments;
		getBuffers(segments, absOffset, innerSize, ACCESS_WRITE, true);

		//copy data
		for (auto & it : segments) {
			//calc offset & size
			size_t destOffset = absOffset - it.offset;
			size_t copySize = it.size;
			if (copySize > innerSize)
				copySize = innerSize;
			
			//copy
			memcpy(it.ptr + destOffset, origSegment.getBuffer() + innerOffset, copySize);

			//move
			innerOffset += copySize;
			absOffset += copySize;
			innerSize -= copySize;
		}
	} else {
		//we make the segment in cow mode
		ObjectSegment & segment = this->segmentMap[origSegment.getOffset() + origSegment.getSize() - 1];
		segment.makeCowOf(origSegment);
	}
}

/****************************************************/
/**
 * Create a copy of the current object in memory and on the remote server with
 * the given new ID. This specific function only copy the given range.
 * @param origObject The source object to copy on the current one (this).
 * @param offset Define the offset from which to copy. If it does not be aligned
 * on pre-existing object segments there will be data copies on border segments.
 * @param size Size of the segment to copy.
**/
void Object::rangeCopyOnWrite(Object & origObject, size_t offset, size_t size)
{
	//search first segment
	auto itTarget = origObject.segmentMap.lower_bound(offset);

	//nothing found
	if (itTarget == origObject.segmentMap.end())
		return;

	//inc if before
	if (itTarget->second.overlap(offset, size) == false)
		itTarget++;

	//loop on all segment to make cow
	while (itTarget != origObject.segmentMap.end() && itTarget->second.overlap(offset, size)) {
		rangeCopyOnWriteSegment(itTarget->second, offset, size);
		++itTarget;
	}
}

/****************************************************/
/**
 * Create a copy of the current object in memory and on the remote server with
 * the given new ID.
 * @param targetObjectId The cow object id to create.
 * @param allowExist Do not fail if the object already exist (fail to create)
**/
Object * Object::makeFullCopyOnWrite(const ObjectId & targetObjectId, bool allowExist)
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

/****************************************************/
/**
 * If can get a uniq segment directly return the pointer starting at the given offset.
 * This is more to unit tests.
 * @param base The base offset from where to start.
 * @param size The size of the desired segment.
 * @param accessMode The mode of access.
 * @param load If need to load the data before returning.
**/
char * Object::getUniqBuffer(size_t base, size_t size, ObjectAccessMode accessMode, bool load)
{
	//get list
	ObjectSegmentList segments;
	this->getBuffers(segments, base, size, accessMode, load);

	//check has only one
	assumeArg(segments.size() == 1, "Fail to get a uniq segment, got %1").arg(segments.size()).end();

	//get ptr
	ObjectSegmentDescr segment = segments.front();
	char * ptr = (char*)segment.ptr;

	//apply offset
	ptr += base - segment.offset;

	//return
	return ptr;
}

/****************************************************/
/**
 * Fill the given range with the given value. This function
 * is implemented to help unit test implementation.
 * @param offset Define the offset from which to start to fill.
 * @param size Define the size of the region to fill.
 * @param value Define the value to be used for filling octets.
**/
void Object::fillBuffer(size_t offset, size_t size, char value)
{
	//touch range on orig
	ObjectSegmentList segments;
	this->getBuffers(segments, offset, size, ACCESS_WRITE);

	//fill
	for (auto & it : segments) {
		//init range
		size_t localOffset = 0;
		size_t localSize = it.size;

		//not full overlap
		if (it.offset < offset) {
			localOffset = offset - it.offset;
			localSize -= localOffset;
		}
		if (localOffset + localSize > size)
			localSize = size - localOffset;

		//set
		memset(it.ptr + localOffset, value, localSize);
	}
}

/****************************************************/
/**
 * Check if the given range is backed by a uniq segment completely
 * filling the given range.
 * @return True if backed by a uniq segment, false if backed by multiple
 * segments of a too small one.
**/
bool Object::checkUniq(size_t offset, size_t size)
{
	ObjectSegmentList lst;
	this->getBuffers(lst, offset , size, ACCESS_READ);
	if (lst.size() != 1)
		return false;
	if (lst.front().offset != offset)
		return false;
	if (lst.front().size != size)
		return false;
	return true;
}

/****************************************************/
/**
 * Check if the given range is filled by the given value.
 * This function aimed to help unit test implementation.
 * @param offset The base offset of the range to test.
 * @param size The size of the range to test.
 * @param value The expected value.
 * @return True if the range is filled by the given value,
 * it return false otherwise.
**/
bool Object::checkBuffer(size_t offset, size_t size, char value)
{
	//touch range on orig
	ObjectSegmentList segments;
	this->getBuffers(segments, offset, size, ACCESS_WRITE);

	//fill
	for (auto & it : segments) {
		//init range
		size_t localOffset = 0;
		size_t localSize = it.size;

		//not full overlap
		if (it.offset < offset) {
			localOffset = offset - it.offset;
			localSize -= localOffset;
		}
		if (localOffset + localSize > size)
			localSize = size;

		//set
		for (size_t i = 0 ; i < localSize ; i++)
			if (it.ptr[localOffset + i] != value)
				return false;
	}

	//ok
	return true;
}
