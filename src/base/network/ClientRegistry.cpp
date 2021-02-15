/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <cassert>
#include "ClientRegistry.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
ClientRegistry::ClientRegistry(void)
{
	//nothing to do
}

/****************************************************/
ClientRegistry::~ClientRegistry(void)
{
	//nothing to do
}

/****************************************************/
void ClientRegistry::registerClient(uint64_t id, uint64_t key)
{
	//CRITICAL SECTION
	{
		std::lock_guard<std::mutex> lockGuard(mutex);
		assert(clients.find(id) == clients.end());
		clients[id] = key;
	}
}

/****************************************************/
void ClientRegistry::disconnectClient(uint64_t id)
{
	//CRITICAL SECTION
	{
		std::lock_guard<std::mutex> lockGuard(mutex);
		assert(clients.find(id) != clients.end());
		clients.erase(id);
	}
}

/****************************************************/
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
