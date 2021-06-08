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
 * @param objectSegmentsAlignement Can setup a minimal size for object segments to get better performances.
**/
Container::Container(StorageBackend * storageBackend, MemoryBackend * memBack, size_t objectSegmentsAlignement)
{
	this->memoryBackend = memBack;
	this->storageBackend = storageBackend;
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
 * Change the memory backend (also applied to all existing objects).
 * @param memoryBackend The new backend to be used.
**/
void Container::setMemoryBackend(MemoryBackend * memoryBackend)
{
	//setup local
	this->memoryBackend = memoryBackend;

	//apply on existing objects
	for (auto & it: this->objects)
		it.second->setMemoryBackend(memoryBackend);
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
		Object * obj = new Object(this->storageBackend, this->memoryBackend, objectId, objectSegmentsAlignement);
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
 * Make a copy on write operation on the given object on the given range.
 * @param sourceId The ID of the source object.
 * @param destId The ID object object to create in COW mode.
 * @param allowExist Do not fail if the object already exist (fail to create) (TODO check if we keep this option)
 * @param offset Base offset from which to start to cow.
 * @param size Size of the segment to cow.
 * @return True if the COW has been makde, false in case of error.
**/
bool Container::makeObjectRangeCow(const ObjectId & sourceId, const ObjectId &destId, bool allowExist, size_t offset, size_t size)
{
	//search
	auto it = objects.find(sourceId);	

	//not found
	if (it == objects.end()) {
		return false;
	}

	//extract source
	Object & sourceObject = *it->second;

	//if dest object already exist
	auto itDest = objects.find(destId);
	Object * destObj = NULL;
	if (itDest != objects.end()) {
		if (allowExist == false)
			return false;
		destObj = itDest->second;
	} else {
		objects[destId] = destObj = new Object(this->storageBackend, this->memoryBackend, destId, this->objectSegmentsAlignement);
	}

	//apply cow on the given range
	destObj->rangeCopyOnWrite(sourceObject, offset, size);

	//ok
	return true;
}

/****************************************************/
/**
 * Make a copy on write operation on the given object.
 * @param sourceId The ID of the source object.
 * @param destId The ID object object to create in COW mode.
 * @param allowExist Do not fail if the object already exist (fail to create)
 * @return True if the COW has been makde, false in case of error.
**/
bool Container::makeObjectFullCow(const ObjectId & sourceId, const ObjectId &destId, bool allowExist)
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
	Object * cowObj = it->second->makeFullCopyOnWrite(destId, allowExist);
	if (cowObj == NULL)
		return false;

	//register
	objects[destId] = cowObj;

	//ok
	return true;
}
