/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_HOOK_RANGE_UNREGISTER_HPP
#define IOC_HOOK_RANGE_UNREGISTER_HPP

/****************************************************/
#include "base/network/Hook.hpp"
#include "../core/Config.hpp"
#include "../core/Container.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement the server side handling of object range unregister operations.
**/
class HookRangeUnregister : public Hook
{
	public:
		HookRangeUnregister(const Config * config, Container * container);
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, LibfabricClientMessage & message) override;
	private:
		/** Pointer to the container to be able to access objects **/
		Container * container;
		/** Keep track of the config object. **/
		const Config * config;
};

}

#endif //IOC_HOOK_RANGE_UNREGISTER_HPP
