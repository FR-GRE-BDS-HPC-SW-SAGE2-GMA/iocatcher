/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <cassert>
#include "Object.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
bool ObjectSegment::overlap(size_t segBase, size_t segSize)
{
	return (this->offset < segBase + segSize && this->offset + this->size > segBase);
}

/****************************************************/
Object::Object(LibfabricDomain * domain, int64_t low, int64_t high)
{
	this->domain = domain;
	this->objectId.low = low;
	this->objectId.high = high;
}

/****************************************************/
const ObjectId & Object::getObjectId(void)
{
	return this->objectId;
}

/****************************************************/
void Object::getBuffers(ObjectSegmentList & segments, size_t base, size_t size)
{
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
			tmp.push_back(this->loadSegment(lastOffset, it.offset - lastOffset));
		}
		lastOffset = it.offset + it.size;
	}

	//merge list
	for (auto it : tmp)
		segments.push_back(it);

	//load last
	if (lastOffset < base + size)
		segments.push_back(this->loadSegment(lastOffset, base + size - lastOffset));
}

/****************************************************/
ObjectSegment Object::loadSegment(size_t offset, size_t size)
{
	ObjectSegment & segment = this->segmentMap[offset];
	segment.offset = offset;
	segment.size = size;
	segment.ptr = (char*)malloc(size);
	if (this->domain != NULL)
		this->domain->registerSegment(segment.ptr, segment.size);
}
