/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "Container.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
Container::Container(LibfabricDomain * lfDomain, size_t objectSegmentsAlignement)
{
	this->lfDomain = lfDomain;
	this->objectSegmentsAlignement = objectSegmentsAlignement;
}

/****************************************************/
Container::~Container(void)
{

}

/****************************************************/
Object & Container::getObject(int64_t high, int64_t low)
{
	//build id
	ObjectId id;
	id.low = low;
	id.high = high;

	//search
	auto it = objects.find(id);

	//if not found or found
	if (it == objects.end()) {
		Object * obj = new Object(lfDomain, low, high, objectSegmentsAlignement);
		objects.emplace(id, obj);
		return *obj;
	} else {
		return *it->second;
	}
}

/****************************************************/
bool Container::hasObject(int64_t high, int64_t low)
{
	//build id
	ObjectId id;
	id.low = low;
	id.high = high;

	//search
	auto it = objects.find(id);

	//ret
	return it != objects.end();
}

/****************************************************/
void Container::onClientDisconnect(uint64_t tcpClientId)
{
	for (auto it : objects)
		it.second->getConsistencyTracker().clientDisconnect(tcpClientId);
}

/****************************************************/
void Container::setObjectSegmentsAlignement(size_t alignement)
{
	this->objectSegmentsAlignement = alignement;
}
