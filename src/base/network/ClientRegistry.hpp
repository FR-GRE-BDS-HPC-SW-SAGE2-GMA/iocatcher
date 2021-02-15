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
#include <mutex>

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
		std::map<uint64_t, uint64_t> clients;
		std::mutex mutex;
};

}

#endif //IOC_CLIENT_REGISTRY_HPP
