/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_HOOK_OBJECT_WRITE_HPP
#define IOC_HOOK_OBJECT_WRITE_HPP

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
class HookObjectWrite : public Hook
{
	public:
		HookObjectWrite(Container * container, ServerStats * stats);
		virtual LibfabricActionResult onMessage(LibfabricConnection * connection, LibfabricClientRequest & request) override;
	private:
		void objRdmaFetchFromClient(LibfabricConnection * connection, uint64_t clientId, LibfabricObjReadWriteInfos & objReadWrite, ObjectSegmentList & segments);
		void objEagerExtractFromMessage(LibfabricConnection * connection, uint64_t clientId, LibfabricObjReadWriteInfos & objReadWrite, ObjectSegmentList & segments);
	private:
		/** Pointer to the container to be able to access objects **/
		Container * container;
		ServerStats * stats;
};

}

#endif //IOC_HOOK_OBJECT_WRITE_HPP
