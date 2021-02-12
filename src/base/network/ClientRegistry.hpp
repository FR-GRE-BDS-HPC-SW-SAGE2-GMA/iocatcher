/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_CLIENT_REGISTRY_HPP
#define IOC_CLIENT_REGISTRY_HPP

/****************************************************/
#include <stdint.h>
#include <map>

/****************************************************/
namespace IOC
{

/****************************************************/
class ClientRegistry
{
	public:
		ClientRegistry(void);
		~ClientRegistry(void);
		void registerClient(uint64_t id, uint64_t key);
		void disconnectClient(uint64_t id);
		bool checkIdentification(uint64_t id, uint64_t key);
	private:
		uint64_t id;
		uint64_t key;
		std::map<uint64_t, uint64_t> clients;
};

}

#endif //IOC_CLIENT_REGISTRY_HPP
