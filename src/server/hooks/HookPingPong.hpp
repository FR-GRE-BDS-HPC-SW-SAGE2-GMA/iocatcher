/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_HOOK_PING_PONG_HPP
#define IOC_HOOK_PING_PONG_HPP

/****************************************************/
#include "base/network/Hook.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement the server side handling of ping-pong operations.
**/
class HookPingPong : public Hook
{
	public:
		HookPingPong(LibfabricDomain * domain);
		~HookPingPong(void);
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, LibfabricClientRequest & request) override;
	private:
		void reallocateBuffer(size_t rdmaSize);
	private:
		char * buffer;
		size_t bufferSize;
		LibfabricDomain * domain;
		Iov bufferIov;
};

}

#endif //IOC_HOOK_PING_PONG_HPP
