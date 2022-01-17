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
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, LibfabricClientRequest & request) override;
};

}

#endif //IOC_HOOK_PING_PONG_HPP
