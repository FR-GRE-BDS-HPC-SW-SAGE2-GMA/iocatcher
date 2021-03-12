/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cstring>
//internal
#include "base/common/Debug.hpp"
#include "ObjectSegment.hpp"
//unix
#include <sys/mman.h>

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the memory tracker.
 * @param buffer Address of the buffer to be tracked (can be NULL for unit tests).
 * @param size Size of the buffer to be tracked (to know how to call munmap()).
 * @param isMmap Declare if the segment has been allocated with mmap() or malloc().
 * @param domain Pointer to the libfabric domain to know how to unregister the memory. Can be NULL for tests.
**/
ObjectSegmentMemory::ObjectSegmentMemory(char * buffer, size_t size, bool isMmap, LibfabricDomain * domain)
{
	//setup
	this->buffer = buffer;
	this->size = size;
	this->isMmap = isMmap;
	this->domain = domain;
}

/****************************************************/
/**
 * Destructor of the memory tracker. It frees the memory using either
 * free() or munmap() depening on the state of isMmap. Do nothing
 * if the buffer pointer is NULL.
**/
ObjectSegmentMemory::~ObjectSegmentMemory(void)
{
	//nothing to do
	if (buffer == nullptr)
		return;

	//unregister
	if (this->domain != nullptr)
		this->domain->unregisterSegment(this->buffer, this->size);

	//free
	if (isMmap) {
		int status = munmap(buffer, size);
		assumeArg(status == 0, "Failed to munmap() the obejct segment: %1.").argStrErrno().end();
	} else {
		free(buffer);
	}
}

/****************************************************/
/**
 * Default constructor, it sets everything to NULL of zero.
**/
ObjectSegment::ObjectSegment(void)
{
	this->memory = nullptr;
	this->offset = 0;
	this->dirty = false;
}

/****************************************************/
/**
 * Construct an object segment.
 * @param offset The offset of the segment in the object.
 * @param size Size of the buffer to be tracked (to know how to call munmap()).
 * @param buffer Address of the buffer to be tracked (can be NULL for unit tests).
 * @param isMmap Declare if the segment has been allocated with mmap() or malloc().
**/
ObjectSegment::ObjectSegment(size_t offset, size_t size, char * buffer, bool isMmap, LibfabricDomain * domain)
{
	this->offset = offset;
	this->dirty = false;
	this->memory = std::make_shared<ObjectSegmentMemory>(buffer, size, isMmap, domain);
}

/****************************************************/
/**
 * Check if the given range overlap with the given segment.
 * @param segBase The base offset of the segment to test.
 * @param segSize The size of the segment to test.
 * @return True if the segment overlap, false otherwise.
**/
bool ObjectSegment::overlap(size_t segBase, size_t segSize)
{
	assert(memory != nullptr);
	return (this->offset < segBase + segSize && this->offset + this->memory->getSize() > segBase);
}

/****************************************************/
/**
 * Build a segment descriptor from the current object segment.
**/
ObjectSegmentDescr ObjectSegment::getSegmentDescr(void)
{
	//check
	assert(this->memory != nullptr);

	//convert
	ObjectSegmentDescr descr = {
		.ptr = this->memory->getBuffer(),
		.offset = this->offset,
		.size = this->memory->getSize()
	};

	//ret
	return descr;
}

/****************************************************/
/**
 * Make the current segment a COW segment of the given origin so it
 * will share the same memory buffer until first write access on one
 * of the two segments.
 * @param orig Reference to the original segment to share.
**/
void ObjectSegment::makeCowOf(ObjectSegment & orig)
{
	this->memory = orig.memory;
	this->dirty = orig.dirty;
	this->offset = orig.offset;
}

/****************************************************/
/**
 * Function to be called on a first write access to a shared COW segment.
 * @param new_ptr Define the new memory address to be used to store data.
 * This function is in charge of copying the data in.
 * @param size Size of the allocated segment for safety check.
 * @param isMmap Declare if the segment has been allocated with mmap() or malloc().
**/
void ObjectSegment::applyCow(char * new_ptr, size_t size, bool isMmap, LibfabricDomain * domain)
{
	//check
	assert(this->memory != nullptr);
	assert(this->memory->getSize() == size);
	assert(this->isCow());
	assert(new_ptr != NULL);

	//copy content
	memcpy(new_ptr, this->memory->getBuffer(), size);

	//override the shared pointer
	this->memory = std::make_shared<ObjectSegmentMemory>(new_ptr, memory->getSize(), isMmap, domain);
}

/****************************************************/
/**
 * Return true if the segment use a shared COW memory region, false otherwise.
**/
bool ObjectSegment::isCow(void)
{
	return !memory.unique();
}
