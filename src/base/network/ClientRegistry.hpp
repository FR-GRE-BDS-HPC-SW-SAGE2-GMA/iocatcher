/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
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
/**
 * The client registry is used to keep track of the connection key to allow
 * a client to talk to the RDMA server part. The keys are exchanged via a first
 * exchange on a TCP connection.
 * @brief Keep track of client authentication keys.
**/
class ClientRegistry
{
	public:
		ClientRegistry(void);
		~ClientRegistry(void);
		void registerClient(uint64_t id, uint64_t key);
		void disconnectClient(uint64_t id);
		bool checkIdentification(uint64_t id, uint64_t key);
	private:
		/** Map joining client IDs and auth keys. **/
		std::map<uint64_t, uint64_t> clients;
		/** 
		 * Mutex as this is accessed by the TCP thread and the main RDMA 
		 * thread.
		**/
		std::mutex mutex;
};

}

#endif //IOC_CLIENT_REGISTRY_HPP
