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
/**
 * Constructor of the container, mostly to set default values.
 * @param lfDomain Provide the libfabric domain to be used for memory registration to be ready for RDMA operations.
 * @param objectSegmentsAlignement Can setup a minimal size for object segments to get better performances.
**/
Container::Container(StorageBackend * storageBackend, LibfabricDomain * lfDomain, size_t objectSegmentsAlignement)
{
	this->storageBackend = storageBackend;
	this->lfDomain = lfDomain;
	this->objectSegmentsAlignement = objectSegmentsAlignement;
}

/****************************************************/
/**
 * Destructor of container, clear all objects before exitint.
**/
Container::~Container(void)
{
	for (auto & it: this->objects)
		delete it.second;
}

/****************************************************/
/**
 * Get an object from its object ID. If not found it will be created.
 * @param high The high part of the object ID.
 * @param low The low part of the object ID.
 * @return A reference to the requested object.
**/
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
		Object * obj = new Object(this->storageBackend, lfDomain, low, high, objectSegmentsAlignement);
		objects.emplace(id, obj);
		return *obj;
	} else {
		return *it->second;
	}
}

/****************************************************/
/**
 * Check the existence of an object.
 * @param high The high part of the object ID.
 * @param low The low part of the object ID.
 * @return True if the object exist, false otherwise.
**/
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
/**
 * On client disconnect, loop on all objects to remove the registration ranges
 * it allocates.
 * @param tcpClientId Identify the client to be removed.
**/
void Container::onClientDisconnect(uint64_t tcpClientId)
{
	for (auto it : objects)
		it.second->getConsistencyTracker().clientDisconnect(tcpClientId);
}

/****************************************************/
/**
 * Set the minimap size of and object segment for better performance.
 * It applies only on new allocated objects.
 * @param alignement The size to setup.
**/
void Container::setObjectSegmentsAlignement(size_t alignement)
{
	this->objectSegmentsAlignement = alignement;
}

/****************************************************/
/**
 * Make a copy on write operation on the given object.
 * @param origLow the low part of the origin object ID.
 * @param origHigh the high part of the origin object ID.
 * @param destLow the low part of the destination object ID.
 * @param destHigh the high part of the destination object ID.
 * @return True if the COW has been makde, false in case of error.
**/
bool Container::makeObjectCow(int64_t origLow, int64_t origHigh, int64_t destLow, int64_t destHigh)
{
	//build id
	ObjectId origId;
	origId.low = destLow;
	origId.high = destHigh;

	//search
	auto it = objects.find(origId);	

	//not found
	if (it == objects.end()) {
		return false;
	}

	//cow
	Object * cowObj = it->second->makeCopyOnWrite(destHigh, destLow);
	if (cowObj == NULL)
		return false;

	//build id
	ObjectId destId;
	destId.low = destLow;
	destId.high = destHigh;

	//register
	objects[destId] = cowObj;

	//ok
	return true;
}
