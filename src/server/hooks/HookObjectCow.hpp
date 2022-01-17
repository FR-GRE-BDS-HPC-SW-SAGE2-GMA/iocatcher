/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_HOOK_OBJECT_COW_HPP
#define IOC_HOOK_OBJECT_COW_HPP

/****************************************************/
#include "base/network/Hook.hpp"
#include "../core/Container.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement the server side handling of object create operations.
**/
class HookObjectCow : public Hook
{
	public:
		HookObjectCow(Container * container);
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, LibfabricClientMessage & message) override;
	private:
		/** Pointer to the container to be able to access objects **/
		Container * container;
};

}

#endif //IOC_HOOK_OBJECT_COW_HPP
