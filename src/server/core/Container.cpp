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
Container::Container(LibfabricDomain * lfDomain)
{
	this->lfDomain = lfDomain;
}

/****************************************************/
Container::~Container(void)
{

}

/****************************************************/
Object & Container::getObject(int64_t low, int64_t high)
{
	//build id
	ObjectId id;
	id.low = low;
	id.high = high;

	//search
	auto it = objects.find(id);

	//if not found or found
	if (it == objects.end()) {
		Object * obj = new Object(lfDomain, low, high);
		objects.emplace(id, obj);
		return *obj;
	} else {
		return *it->second;
	}
}

