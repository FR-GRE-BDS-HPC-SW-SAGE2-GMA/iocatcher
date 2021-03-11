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
class ObjectSegmentMemory
{
	public:
		ObjectSegmentMemory(char * buffer, size_t size, bool isMmap);
		~ObjectSegmentMemory(void);
		char * getBuffer(void) {return this->buffer;};
		size_t getSize(void) {return this->size;}
	private:
		char * buffer;
		size_t size;
		bool isMmap;
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
		ObjectSegment(size_t offset, size_t size, char * buffer, bool isMmap);
		bool overlap(size_t segBase, size_t segSize);
		ObjectSegmentDescr getSegmentDescr(void);
		size_t getSize(void) {assert(memory != nullptr); return this->memory->getSize();};
		size_t getOffset(void) {return this->offset;};
		bool isDirty(void) {return this->dirty;};
		void setDirty(bool value) {this->dirty = value;};
		char * getBuffer(void) {assert(memory != nullptr); return this->memory->getBuffer();};
		void makeCowOf(ObjectSegment & orig);
		void applyCow(char * new_ptr, size_t size, bool isMmap);
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
