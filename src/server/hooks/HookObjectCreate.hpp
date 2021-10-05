/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_HOOK_OBJECT_CREATE_HPP
#define IOC_HOOK_OBJECT_CREATE_HPP

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
class HookObjectCreate : public Hook
{
	public:
		HookObjectCreate(Container * container);
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, uint64_t lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage) override;
	private:
		/** Pointer to the container to be able to access objects **/
		Container * container;
};

}

#endif //IOC_HOOK_OBJECT_CREATE_HPP
