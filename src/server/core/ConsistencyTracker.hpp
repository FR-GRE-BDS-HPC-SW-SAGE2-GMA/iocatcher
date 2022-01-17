/*****************************************************
/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_CONSISTENCY_TRACKER_HPP
#define IOC_CONSISTENCY_TRACKER_HPP

/****************************************************/
#include <list>
#include <mutex>
#include <cstdlib>
#include <stdint.h>

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Mapping ranges can be read or read-write mode. Write mode
 * are exclusive and allow a uniq mapping at a time.
**/
enum ConsistencyAccessMode
{
	/** The mapping rang is in read mode and can be mapped many times. **/
	CONSIST_ACCESS_MODE_READ,
	/** The mapping rang is in read-write mode and can be mapped a uniq time (exclusive). **/
	CONSIST_ACCESS_MODE_WRITE,
};

/****************************************************/
/**
 * Define a mapping range to be tracked.
**/
struct ConsistencyRange
{
	/** The TCP client ID which host the mapping. **/
	uint64_t tcpClientId;
	/** ID of the mapping to be used for de-registration. **/
	int32_t id;
	/** Offset of the mapping range. **/
	size_t offset;
	/** Size of the mapping range. **/
	size_t size;
	/** Access mode of the mapping. **/
	ConsistencyAccessMode accessMode;
};

/****************************************************/
/**
 * Object used to make the consistency checkings and track the mapping ranges
 * registered by clients. It permits to check consistency enforcement and to
 * automatically de-register the mappings ranges when a client disconnect.
**/
class ConsistencyTracker
{
	public:
		ConsistencyTracker(void);
		~ConsistencyTracker(void);
		int32_t registerRange(uint64_t tcpClientId, size_t offset, size_t size, ConsistencyAccessMode accessMode);
		bool hasCollision(size_t offset, size_t size, ConsistencyAccessMode accessMode);
		bool unregisterRange(uint64_t tcpClientId, int32_t id, size_t offset, size_t size,ConsistencyAccessMode accessMode);
		static bool overlap(size_t offset1, size_t size1, size_t offset2, size_t size2);
		void clientDisconnect(uint64_t tcpClientId);
	private:
		/** 
		 * Mutex to protect the object as it is accessed by the main thread 
		 * (libfabric poller) and the tcp libevent thread.
		**/
		std::mutex mutex;
		/** List of ranges to be tracked. **/
		std::list<ConsistencyRange> ranges;
		/** Next ID to assign, to be incremented for every new range. **/
		int32_t nextId;
};

}

#endif //IOC_CONSISTENCY_TRACKER_HPP
