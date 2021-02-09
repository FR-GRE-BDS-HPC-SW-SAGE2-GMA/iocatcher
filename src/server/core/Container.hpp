/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_CONTAINER_HPP
#define IOC_CONTAINER_HPP

/****************************************************/
#include <cstdint>
#include <cstdlib>
#include <map>
#include "Object.hpp"
#include "../../base/network/LibfabricDomain.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
class Container
{
	public:
		Container(LibfabricDomain * lfDomain, size_t objectSegmentsAlignement = 0);
		~Container(void);
		Object & getObject(int64_t low, int64_t high);
	private:
		std::map<ObjectId, Object*> objects;
		LibfabricDomain * lfDomain;
		size_t objectSegmentsAlignement;
};

}

#endif //IOC_CONTAINER_HPP
