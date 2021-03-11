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
ObjectSegmentMemory::ObjectSegmentMemory(char * buffer, size_t size, bool isMmap)
{
	//setup
	this->buffer = buffer;
	this->size = size;
	this->isMmap = isMmap;
}

/****************************************************/
ObjectSegmentMemory::~ObjectSegmentMemory(void)
{
	//nothing to do
	if (buffer == NULL)
		return;

	//free
	if (isMmap) {
		int status = munmap(buffer, size);
		assumeArg(status == 0, "Failed to munmap() the obejct segment: %1.").argStrErrno().end();
	} else {
		free(buffer);
	}
}

/****************************************************/
ObjectSegment::ObjectSegment(void)
{
	this->memory = nullptr;
	this->offset = 0;
	this->dirty = false;
}

/****************************************************/
ObjectSegment::ObjectSegment(size_t offset, size_t size, char * buffer, bool isMmap)
{
	this->offset = offset;
	this->dirty = false;
	this->memory = std::make_shared<ObjectSegmentMemory>(buffer, size, isMmap);
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
void ObjectSegment::makeCowOf(ObjectSegment & orig)
{
	this->memory = orig.memory;
	this->dirty = orig.dirty;
	this->offset = orig.offset;
}

/****************************************************/
void ObjectSegment::applyCow(char * new_ptr, size_t size, bool isMmap)
{
	//check
	assert(this->memory != nullptr);
	assert(this->memory->getSize() == size);
	assert(new_ptr != NULL);

	//copy content
	memcpy(new_ptr, this->memory->getBuffer(), size);

	//override the shared pointer
	this->memory = std::make_shared<ObjectSegmentMemory>(new_ptr, memory->getSize(), isMmap);
}

/****************************************************/
bool ObjectSegment::isCow(void)
{
	return !memory.unique();
}
