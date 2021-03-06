/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_SERVER_STATS_HPP
#define IOC_SERVER_STATS_HPP

/****************************************************/
//std
#include <cstdlib>

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Struct used to track server statistics.
**/
struct ServerStats
{
	ServerStats(void);
	/** How much bytes we read. **/
	size_t readSize;
	/** how much bytes we wrote. **/
	size_t writeSize;
};

}

#endif //IOC_SERVER_STATS_HPP
