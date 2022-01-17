/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_HOOK_RANGE_REGISTER_HPP
#define IOC_HOOK_RANGE_REGISTER_HPP

/****************************************************/
#include "base/network/Hook.hpp"
#include "../core/Config.hpp"
#include "../core/Container.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement the server side handling of object range register operations.
**/
class HookRangeRegister : public Hook
{
	public:
		HookRangeRegister(const Config * config, Container * container);
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, uint64_t lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage) override;
	private:
		/** Pointer to the container to be able to access objects **/
		Container * container;
		/** Keep track of the config object. **/
		const Config * config;
};

}

#endif //IOC_HOOK_RANGE_REGISTER_HPP
