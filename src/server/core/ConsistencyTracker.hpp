/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_CONSISTENCY_TRACKER_HPP
#define IOC_CONSISTENCY_TRACKER_HPP

/****************************************************/
#include <list>
#include <cstdlib>
#include <stdint.h>

/****************************************************/
namespace IOC
{

/****************************************************/
enum ConsistencyAccessMode
{
	CONSIST_ACCESS_MODE_READ,
	CONSIST_ACCESS_MODE_WRITE,
};

/****************************************************/
struct ConsistencyRange
{
	uint64_t clientId;
	int32_t id;
	size_t offset;
	size_t size;
	ConsistencyAccessMode accessMode;
};

/****************************************************/
class ConsistencyTracker
{
	public:
		ConsistencyTracker(void);
		~ConsistencyTracker(void);
		int32_t registerRange(uint64_t clientId, size_t offset, size_t size, ConsistencyAccessMode accessMode);
		bool hasCollision(size_t offset, size_t size, ConsistencyAccessMode accessMode);
		bool unregisterRange(uint64_t clientId, int32_t id, size_t offset, size_t size,ConsistencyAccessMode accessMode);
		static bool overlap(size_t offset1, size_t size1, size_t offset2, size_t size2);
		void clientDisconnect(uint64_t clientId);
	private:
		std::list<ConsistencyRange> ranges;
		int32_t nextId;
};

}

#endif //IOC_CONSISTENCY_TRACKER_HPP
