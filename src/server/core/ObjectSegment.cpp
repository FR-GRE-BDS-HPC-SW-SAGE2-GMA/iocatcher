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
 * @param memoryBackend To know how to free the memory on deletion.
**/
ObjectSegmentMemory::ObjectSegmentMemory(char * buffer, size_t size, MemoryBackend * memoryBackend)
{
	//setup
	this->buffer = buffer;
	this->size = size;
	this->memoryBackend = memoryBackend;
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

	//free
	this->memoryBackend->deallocate(buffer, size);
}

/****************************************************/
/**
 * Return the current memory backend.
**/
MemoryBackend * ObjectSegmentMemory::getMemoryBackend(void)
{
	return this->memoryBackend;
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
ObjectSegment::ObjectSegment(size_t offset, size_t size, char * buffer, MemoryBackend * memoryBackend)
{
	this->offset = offset;
	this->dirty = false;
	this->memory = std::make_shared<ObjectSegmentMemory>(buffer, size, memoryBackend);
}

/****************************************************/
/**
 * Check if the given range overlap with the given segment.
 * @param segBase The base offset of the segment to test.
 * @param segSize The size of the segment to test.
 * @return True if the segment overlap, false otherwise.
**/
bool ObjectSegment::overlap(size_t segBase, size_t segSize) const
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
**/
void ObjectSegment::applyCow(void)
{
	//check
	assert(this->memory != nullptr);
	assert(this->isCow());
	
	//vars
	const size_t size = this->memory->getSize();
	MemoryBackend * memoryBackend = this->memory->getMemoryBackend();

	//allocate new ptr
	char * new_ptr = (char*)memoryBackend->allocate(size);
	assert(new_ptr != NULL);

	//copy content
	memcpy(new_ptr, this->memory->getBuffer(), size);

	//override the shared pointer
	this->memory = std::make_shared<ObjectSegmentMemory>(new_ptr, size, memoryBackend);
}

/****************************************************/
/**
 * Return true if the segment use a shared COW memory region, false otherwise.
**/
bool ObjectSegment::isCow(void)
{
	return !memory.unique();
}
