/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_HOOK_FLUSH_HPP
#define IOC_HOOK_FLUSH_HPP

/****************************************************/
#include "base/network/Hook.hpp"
#include "../core/Container.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement the server side handling of object flush operations.
**/
class HookFlush : public Hook
{
	public:
		HookFlush(Container * container);
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, LibfabricClientRequest & request) override;
	private:
		/** Pointer to the container to be able to access objects **/
		Container * container;
};

}

#endif //IOC_HOOK_FLUSH_HPP
