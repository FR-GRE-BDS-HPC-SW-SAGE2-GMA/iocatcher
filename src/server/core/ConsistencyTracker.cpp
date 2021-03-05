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
/**
 * Constructor of the consistency tracker, it mostly setup the default values.
**/
ConsistencyTracker::ConsistencyTracker(void)
{
	this->nextId = 1;
}

/****************************************************/
/**
 * Destructor of the consitency tracker, currently do nothing.
**/
ConsistencyTracker::~ConsistencyTracker(void)
{
}

/****************************************************/
/**
 * Check if has a collision before establishing a new mapping.
 * @param offset Offset of the range to test.
 * @param size Size of the range to test.
 * @param accessMode Define the wanted access mode to check write exclusivity.
 * @return Return true if there is a collision.
**/
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
/**
 * Registry a new mapping range if there is no collision.
 * @param tcpClientId The TCP client ID making the request.
 * @param offset Offset of the range to register.
 * @param size Size of the range to register.
 * @param accessMode Define the access mode to check access exclusivity.
 * @return The range ID if there is no collision and -1 if there is a collision.
**/
int32_t ConsistencyTracker::registerRange(uint64_t tcpClientId, size_t offset, size_t size, ConsistencyAccessMode accessMode)
{
	//CRITICAL SECTION
	{
		std::lock_guard<std::mutex> guard(this->mutex);
	
		//check collision
		if (this->hasCollision(offset, size, accessMode))
			return -1;

		//register
		struct ConsistencyRange range = {
			.tcpClientId = tcpClientId,
			.id = this->nextId++,
			.offset = offset,
			.size = size,
			.accessMode = accessMode
		};
		this->ranges.push_back(range);

		//ok
		return range.id;
	}
}

/****************************************************/
/**
 * Remove a range registration from the register.
 * @param tcpClientId The TCP client ID making the request, need to match with one of the regitered ranges.
 * @param id ID of the segment, need to match with one of the regitered ranges.
 * @param offset Offset of the range, need to match with one of the regitered ranges.
 * @param size Size of the range, need to match with one of the regitered ranges.
 * @param accessMode Access mode of the range, need to match with one of the regitered ranges.
 * @return True if removed by success, false if not found.
**/
bool ConsistencyTracker::unregisterRange(uint64_t tcpClientId, int32_t id, size_t offset, size_t size, ConsistencyAccessMode accessMode)
{
	//CRITICAL SECTION
	{
		std::lock_guard<std::mutex> guard(this->mutex);
	
		//loop to find overlap
		for (auto it = ranges.begin(); it != ranges.end(); ++it) {
			if (it->tcpClientId == tcpClientId && it->offset == offset && it->size == size && it->accessMode == accessMode && it->id == id) {
				ranges.erase(it);
				return true;
			}
		}

		return false;
	}
}

/****************************************************/
/**
 * Check if taw ranges are overlapping.
 * @param offset1 Offset of the mapping 1.
 * @param size1 Size of the mapping 1.
 * @param offset2 Offset of thr mapping 2.
 * @param size2 Size of the mapping 2.
 * @return True if there is an overlap, false otherwise.
**/
bool ConsistencyTracker::overlap(size_t offset1, size_t size1, size_t offset2, size_t size2)
{
	if (offset1 >= offset2 && offset1 < offset2 + size2)
		return true;
	if (offset2 >= offset1 && offset2 < offset1 + size1)
		return true;
	return false;
}

/****************************************************/
/**
 * Function to be called when a client disconnect to remove all the
 * ranges he registered.
 * @param tcpClientId The TCP client ID of the client being diconnected.
**/
void ConsistencyTracker::clientDisconnect(uint64_t tcpClientId)
{
	//CRITICAL SECTION
	{
		std::lock_guard<std::mutex> guard(this->mutex);

		for (auto it = ranges.begin() ; it != ranges.end() ; ) {
			if (it->tcpClientId == tcpClientId)
				it = ranges.erase(it);
			else
				++it;
		}
	}
}
