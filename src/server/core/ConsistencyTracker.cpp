/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "ConsistencyTracker.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
ConsistencyTracker::ConsistencyTracker(void)
{
	this->nextId = 1;
}

/****************************************************/
ConsistencyTracker::~ConsistencyTracker(void)
{
}

/****************************************************/
bool ConsistencyTracker::hasCollision(size_t offset, size_t size, ConsistencyAccessMode accessMode)
{
	//check all
	for (auto & it : ranges) {
		if (overlap(offset, size, it.offset, it.size) && (accessMode != it.accessMode || it.accessMode == CONSIST_ACCESS_MODE_WRITE))
			return true;
	}

	//no collision
	return false;
}

/****************************************************/
int32_t ConsistencyTracker::registerRange(size_t offset, size_t size, ConsistencyAccessMode accessMode)
{
	//check collision
	if (this->hasCollision(offset, size, accessMode))
		return -1;

	//register
	struct ConsistencyRange range = {
		.id = this->nextId++,
		.offset = offset,
		.size = size,
		.accessMode = accessMode
	};
	this->ranges.push_back(range);

	//ok
	return range.id;
}

/****************************************************/
bool ConsistencyTracker::unregisterRange(int32_t id, size_t offset, size_t size, ConsistencyAccessMode accessMode)
{
	//loop to find overlap
	for (auto it = ranges.begin(); it != ranges.end(); ++it) {
		if (it->offset == offset && it->size == size && it->accessMode == accessMode && it->id == id) {
			ranges.erase(it);
			return true;
		}
	}

	return false;
}

/****************************************************/
bool ConsistencyTracker::overlap(size_t offset1, size_t size1, size_t offset2, size_t size2)
{
	if (offset1 >= offset2 && offset1 < offset2 + size2)
		return true;
	if (offset2 >= offset1 && offset2 < offset1 + size1)
		return true;
	return false;
}
