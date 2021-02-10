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
		bool registerRange(size_t offset, size_t size, ConsistencyAccessMode accessMode);
		bool hasCollision(size_t offset, size_t size, ConsistencyAccessMode accessMode);
		bool unregisterRange(size_t offset, size_t size);
		static bool overlap(size_t offset1, size_t size1, size_t offset2, size_t size2);
	private:
		std::list<ConsistencyRange> ranges;
};

}

#endif //IOC_CONSISTENCY_TRACKER_HPP
