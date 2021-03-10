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

/* This value is fixed based on the HW that you run, 
 * 50 for VM execution and 25 for Juelich prototype execution has been tested OK
 * Just a bit of explanation here: 
 * Internally, MERO is splitting the write ops into data units 
 * There is an arbitrary limitation within on the number of data units
 * that could be written in a single MERO ops. 
 * 
 * An error on the IO size limitation overhead looks like this: 
 * mero[XXXX]:  eb80  ERROR  [clovis/io_req.c:452:ioreq_iosm_handle_executed]  iro_dgmode_write() failed, rc=-7
 * 
 * A data unit size is directly based on the layout id of the object
 * Default layout id (defined in clovis config) for this example is 9 which leads to a data units size of 1MB
 * A value of 50 means that we can write up to 50MB (50 data units of 1MB) per ops
*/
#ifndef CLOVIS_MAX_DATA_UNIT_PER_OPS
	#define CLOVIS_MAX_DATA_UNIT_PER_OPS  50
#endif

/****************************************************/
using namespace IOC;
using namespace std;

/****************************************************/
/**
 * Store the list of paths to create nvdimm files. Shared between all objects.
**/
std::vector<std::string> IOC::Object::nvdimmPaths;

/****************************************************/
/**
 * Check if the given range overlap with the given segment.
 * @param segBase The base offset of the segment to test.
 * @param segSize The size of the segment to test.
 * @return True if the segment overlap, false otherwise.
**/
bool ObjectSegment::overlap(size_t segBase, size_t segSize)
{
	return (this->offset < segBase + segSize && this->offset + this->size > segBase);
}

/****************************************************/
/**
 * Constructor of an object.
 * @param storageBackend Pointer to the storage backend to be used to load/save data.
 * @param domain The libfabric domain to be used for memory registration.
 * @param objectId The identifier of the object.
 * @param alignement The segment size alignement to be used.
**/
Object::Object(StorageBackend * storageBackend, LibfabricDomain * domain, const ObjectId & objectId, size_t alignement)
{
	this->storageBackend = storageBackend;
	this->alignement = alignement;
	this->domain = domain;
	this->objectId = objectId;
	this->nvdimmId = 0;
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
			it.second.dirty = true;
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
 * @param load If need to load the segment if not present.
**/
void Object::getBuffers(ObjectSegmentList & segments, size_t base, size_t size, bool load)
{
	//align
	if (this->alignement > 0)  {
		size += base % alignement;
		base -= base % alignement;
		if (size % alignement > 0)
			size += alignement - (size % alignement);
	}

	//extract
	for (auto it : this->segmentMap) {
		//if overlap
		if (it.second.overlap(base, size)) {
			segments.push_back(it.second);
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
 * Helper function to allocate an nvdimm stores buffer for the given range.
 * @param nvdimmPath Where to store the object.
 * @param high The high part of the object ID.
 * @param low The low part of the object ID.
 * @param offset The offset of the given range.
 * @param size The size of the range (which define the size of the allocation).
 * @return Address of the allocated segment.
**/
static char * allocateNvdimm(const std::string & nvdimmPath, int64_t high, int64_t low, size_t offset, size_t size)
{
	//open
	char fname[1024];
	sprintf(fname, "%s/ioc-%ld:%ld-%lu.raw", nvdimmPath.c_str(), high, low, offset);
	unlink(fname);
	int fd = open(fname, O_CREAT|O_RDWR);
	assume(fd >= 0, "Fail to open nvdimm file !");

	//truncate
	//printf("%d %lu\n", fd, size);
	int status = ftruncate(fd, size);
	assumeArg(status == 0, "Fail to ftruncate: %1").argStrErrno().end();

	//mmap
	void * ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	assumeArg(ptr != MAP_FAILED, "Fail to mmap : %1").argStrErrno().end();
	memset(ptr, 0, size);

	//madvie
	//status = madvise(ptr, size, MADV_DONTFORK);
	//assumeArg(status != 0, "Fail to madvise: %1").argStrErrno().end();

	return (char*)ptr;
}

/****************************************************/
/**
 * Allocate memory to store data to be served.
 * @param offset The offset of the segment in the object.
 * @param size The size of the segment in the object.
 * @return The memory address of the allocated segment.
**/
char * Object::allocateMem(size_t offset, size_t size)
{
	//vars
	char * ptr = NULL;

	//select the right source
	if (this->nvdimmPaths.empty()) {
		ptr = (char*)malloc(size);
	} else {
		ptr = allocateNvdimm(this->nvdimmPaths[this->nvdimmId], this->objectId.high, this->objectId.low, offset, size);
		this->nvdimmId = (this->nvdimmId + 1) % this->nvdimmPaths.size();
	}

	//memory register for RDMA
	if (this->domain != NULL)
		this->domain->registerSegment(ptr, size, true, true, true);
	
	//rep
	return ptr;
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
ObjectSegment Object::loadSegment(size_t offset, size_t size, bool load)
{
	ObjectSegment & segment = this->segmentMap[offset];
	segment.offset = offset;
	segment.size = size;
	segment.dirty = false;
	segment.ptr = allocateMem(offset, size);
	if (load) {
		size_t status = this->pread(this->objectId.high, this->objectId.low, segment.ptr, size, offset);
		assume(status == size, "Fail to helperPread from object !");
		if (status != size)
			status = this->pwrite(this->objectId.high, this->objectId.low, segment.ptr, size, offset);
		assume(status == size, "Fail to write to object !");
	}
	return segment;
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
	for (auto it : this->segmentMap) {
		if (it.second.dirty) {
			if (size == 0 || it.second.overlap(offset, size)) {
				if (this->pwrite(this->objectId.high, this->objectId.low, it.second.ptr, it.second.size, it.second.offset) != (ssize_t)it.second.size)
					ret = -1;
				it.second.dirty = false;
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
ssize_t Object::pwrite(int64_t high, int64_t low, void * buffer, size_t size, size_t offset)
{
	if (this->storageBackend != NULL)
		return this->storageBackend->pwrite(high, low, buffer, size, offset);
	else
		return size;
}

/****************************************************/
/** Just a wrapper to check if we have a storage backend or not. **/
ssize_t Object::pread(int64_t high, int64_t low, void * buffer, size_t size, size_t offset)
{
	if (this->storageBackend != NULL)
		return this->storageBackend->pread(high, low, buffer, size, offset);
	else
		return size;
}

/****************************************************/
/**
 * Permit to order the ranges to use in std::map
**/
bool IOC::operator< (const ObjectSegment & seg1, const ObjectSegment & seg2)
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
 * Set the nvdimm paths.
**/
void IOC::Object::setNvdimm(const std::vector<std::string> & paths)
{
	Object::nvdimmPaths = paths;
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
 * @param high The high part of the object ID.
 * @param low The low part of the object ID.
**/
Object * Object::makeCopyOnWrite(uint64_t high, uint64_t low)
{
	//object ids
	ObjectId destId(high, low);

	//spawn the new object
	Object * cow = new Object(storageBackend, domain, destId, alignement);

	//Create
	int createStatus = cow->create();
	assume(createStatus == 0, "Failed to create object on the storage for COW !");

	//loop on all segments
	size_t cursor = 0;
	for (auto & it : this->segmentMap) {
		//if need to copy non loaded part make a copy on the sorage
		if (cursor < it.second.offset) {
			size_t copySize = it.second.offset - cursor;
			ssize_t status = storageBackend->makeCowSegment(this->objectId.high, this->objectId.low, high, low, cursor, copySize);
			assume(status == (ssize_t)copySize, "Fail to copy on write on the storage for COW !");
		}

		//we copy the segment locally
		ObjectSegment & segment = cow->segmentMap[it.second.offset];
		segment = it.second;

		//make a copy of the memory data in a new segment
		segment.ptr = cow->allocateMem(segment.offset, segment.size);
		memcpy(segment.ptr, it.second.ptr, segment.size);

		//if not dirty we flush
		if (segment.dirty == false) {
			ssize_t status = storageBackend->pwrite(high, low, segment.ptr, segment.size, segment.offset);
			assume(status == (ssize_t)segment.size, "Fail to copy the COW non dirty elements !");
		}

		//move
		cursor = it.second.offset + it.second.size;
	}

	//ret
	return cow;
}
