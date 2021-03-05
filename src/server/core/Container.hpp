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
/**
 * A container aggregate all the handle objects and provide the necessary
 * function to find them from object IDs.
**/
class Container
{
	public:
		Container(LibfabricDomain * lfDomain, size_t objectSegmentsAlignement = 0);
		~Container(void);
		Object & getObject(int64_t low, int64_t high);
		bool hasObject(int64_t low, int64_t high);
		void onClientDisconnect(uint64_t clientId);
		void setObjectSegmentsAlignement(size_t alignement);
	private:
		/** List ob objects identified by their object ID. **/
		std::map<ObjectId, Object*> objects;
		/** Libfabric domain to be used to pre-register the allocated memory to be ready for RDMA operations. **/
		LibfabricDomain * lfDomain;
		/** We can force a minimal size for the object segments to get better performance. **/
		size_t objectSegmentsAlignement;
};

}

#endif //IOC_CONTAINER_HPP
