/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_HOOK_OBJECT_READ_HPP
#define IOC_HOOK_OBJECT_READ_HPP

/****************************************************/
#include "base/network/Hook.hpp"
#include "../core/Container.hpp"
#include "../core/ServerStats.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Implement the server side handling of object flush operations.
**/
class HookObjectRead : public Hook
{
	public:
		HookObjectRead(Container * container, ServerStats * stats);
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, int lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage) override;
	private:
		void objRdmaPushToClient(LibfabricConnection * connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
		void objEagerPushToClient(LibfabricConnection * connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments);
	private:
		/** Pointer to the container to be able to access objects **/
		Container * container;
		ServerStats * stats;
};

}

#endif //IOC_HOOK_OBJECT_READ_HPP
