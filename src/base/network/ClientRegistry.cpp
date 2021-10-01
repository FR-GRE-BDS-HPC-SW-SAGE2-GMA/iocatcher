/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <cassert>
#include "base/common/Debug.hpp"
#include "ClientRegistry.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor, don't to anything yet.
**/
ClientRegistry::ClientRegistry(void)
{
	//nothing to do
}

/****************************************************/
/**
 * Destructeur, don't to anything yet.
**/
ClientRegistry::~ClientRegistry(void)
{
	//nothing to do
}

/****************************************************/
/**
 * Register a client.
 * @param id ID of the client.
 * @param key Authentication key attached to the given ID.
**/
void ClientRegistry::registerClient(uint64_t id, uint64_t key)
{
	//CRITICAL SECTION
	{
		std::lock_guard<std::mutex> lockGuard(mutex);
		assert(clients.find(id) == clients.end());
		IOC_DEBUG_ARG("client:registry", "Register client id=%1, key=%2").arg(id).arg(key).end();
		clients[id] = key;
	}
}

/****************************************************/
/**
 * Disconnect the given client id.
 * @param id of the client to disconnect.
**/
void ClientRegistry::disconnectClient(uint64_t id)
{
	//CRITICAL SECTION
	{
		std::lock_guard<std::mutex> lockGuard(mutex);
		assert(clients.find(id) != clients.end());
		IOC_DEBUG_ARG("client:registry", "Remove client id=%1").arg(id).end();
		clients.erase(id);
	}
}

/****************************************************/
/**
 * Check the authentication of the incomming client identified by its id.
 * @param id ID of the client.
 * @param key Authentication key to check.
 * @return True if the authentication is OK, false otherwise.
**/
bool ClientRegistry::checkIdentification(uint64_t id, uint64_t key)
{
	//CRITICAL SECTION
	{
		std::lock_guard<std::mutex> lockGuard(mutex);

		//search
		auto it = clients.find(id);

		//check ok
		if (it == clients.end())
			return false;

		//check key
		return (it->second == key);
	}
}
