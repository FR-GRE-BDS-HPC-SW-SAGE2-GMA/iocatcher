/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_OBJECT_SEGMENT_HPP
#define IOC_OBJECT_SEGMENT_HPP

/****************************************************/
//std
#include <cstdlib>
#include <memory>
#include <cassert>
//intenral
#include "MemoryBackend.hpp"
#include <base/network/LibfabricDomain.hpp>

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Shotrly represent a segment to be returned via getBuffers().
**/
struct ObjectSegmentDescr
{
	/** Address of the memory buffer storing this segment. **/
	char * ptr;
	/** Offset of this segment. **/
	size_t offset;
	/** Size of this segment. **/
	size_t size;
};

/****************************************************/
/**
 * Keep track of a memory space to store data for the ObjectSegment.
 * Its goal is to handle free operation when used in a std::shared_ptr.
 * It also handle both allocation cases via malloc() or mmap().
**/
class ObjectSegmentMemory
{
	public:
		ObjectSegmentMemory(char * buffer, size_t size, MemoryBackend * memoryBackend);
		~ObjectSegmentMemory(void);
		const char * getBuffer(void) const {return this->buffer;};
		char * getBuffer(void) {return this->buffer;};
		size_t getSize(void) {return this->size;}
		MemoryBackend * getMemoryBackend(void);
	private:
		/** Keep track of the buffer address, can be NULL for none (for unit tests). **/
		char * buffer;
		/** Size of the buffer to know how to call munmap(). **/
		size_t size;
		/** Keep track of the memory backend to know how to free. ***/
		MemoryBackend * memoryBackend;
};

/****************************************************/
/**
 * Define an object segment. It match with what has been requested by clients
 * via read/write operations.
**/
class ObjectSegment
{
	public:
		ObjectSegment(void);
		ObjectSegment(ObjectSegment && orig) = default;
		ObjectSegment(size_t offset, size_t size, char * buffer, MemoryBackend * memoryBackend);
		bool overlap(size_t segBase, size_t segSize) const;
		ObjectSegmentDescr getSegmentDescr(void);
		size_t getSize(void) const {assert(memory != nullptr); return this->memory->getSize();};
		size_t getOffset(void) const {return this->offset;};
		bool isDirty(void) {return this->dirty;};
		void setDirty(bool value) {this->dirty = value;};
		char * getBuffer(void) {assert(memory != nullptr); return this->memory->getBuffer();};
		const char * getBuffer(void) const {assert(memory != nullptr); return this->memory->getBuffer();};
		void makeCowOf(ObjectSegment & orig);
		void applyCow(void);
		ObjectSegment & operator=(ObjectSegment && orig) = default;
		bool isCow(void);
	private:
		/** Address of the memory buffer storing this segment. **/
		std::shared_ptr<ObjectSegmentMemory> memory;
		/** Offset of this segment. **/
		size_t offset;
		/** Store ditry state to know if we need to flush it or not. **/
		bool dirty;
};

}

#endif //IOC_OBJECT_SEGMENT_HPP
