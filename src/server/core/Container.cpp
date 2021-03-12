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
 * Change the storage backend (also applied to all existing objects).
 * @param storageBackend The new backend to be used.
**/
void Container::setStorageBackend(StorageBackend * storageBackend)
{
	//setup local
	this->storageBackend = storageBackend;

	//apply on existing objects
	for (auto & it: this->objects)
		it.second->setStorageBackend(storageBackend);
}

/****************************************************/
/**
 * Get an object from its object ID. If not found it will be created.
 * @param objectId The object ID to create.
 * @return A reference to the requested object.
**/
Object & Container::getObject(const ObjectId & objectId)
{
	//search
	auto it = objects.find(objectId);

	//if not found or found
	if (it == objects.end()) {
		Object * obj = new Object(this->storageBackend, lfDomain, objectId, objectSegmentsAlignement);
		objects.emplace(objectId, obj);
		return *obj;
	} else {
		return *it->second;
	}
}

/****************************************************/
/**
 * Check the existence of an object.
 * @param objectId The object identifier we want to check.
 * @return True if the object exist, false otherwise.
**/
bool Container::hasObject(const ObjectId & objectId)
{
	//search
	auto it = objects.find(objectId);

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
 * @param sourceId The ID of the source object.
 * @param destId The ID object object to create in COW mode.
 * @param allowExist Do not fail if the object already exist (fail to create)
 * @return True if the COW has been makde, false in case of error.
**/
bool Container::makeObjectCow(const ObjectId & sourceId, const ObjectId &destId, bool allowExist)
{
	//search
	auto it = objects.find(sourceId);	

	//not found
	if (it == objects.end()) {
		return false;
	}

	//if dest object already exist
	auto itDest = objects.find(destId);
	if (itDest != objects.end()) {
		if (allowExist == false)
			return false;
		delete itDest->second;
		itDest->second = NULL;
	}

	//cow
	Object * cowObj = it->second->makeCopyOnWrite(destId, allowExist);
	if (cowObj == NULL)
		return false;

	//register
	objects[destId] = cowObj;

	//ok
	return true;
}
