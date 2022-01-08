/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include "base/common/Debug.hpp"
#include "base/network/LibfabricConnection.hpp"
#include "HookRangeUnregister.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the range register hook.
 * @param config The config object to know if range tracking is enabled.
 * @param container The container to be able to access objects to register ranges.
**/
HookRangeUnregister::HookRangeUnregister(const Config * config, Container * container)
{
	this->config = config;
	this->container = container;
}

/****************************************************/
LibfabricActionResult HookRangeUnregister::onMessage(LibfabricConnection * connection, LibfabricClientRequest & request)
{
	//extract
	LibfabricUnregisterRange unregisterRange;
	request.deserializer.apply("unregisterRange", unregisterRange);

	//debug
	IOC_DEBUG_ARG("hook:range:unregister", "Get range unregister %1 from client %5")
		.arg(Serializer::stringify(unregisterRange.id))
		.arg(request.lfClientId)
		.end();

	//get object
	Object & object = this->container->getObject(unregisterRange.objectId);
	ConsistencyTracker & tracker = object.getConsistencyTracker();

	//check
	int status = 0;
	ConsistencyAccessMode mode = CONSIST_ACCESS_MODE_READ;
	if (unregisterRange.write)
		mode = CONSIST_ACCESS_MODE_WRITE;
	if (this->config->consistencyCheck)
		if (!tracker.unregisterRange(request.header->tcpClientId, unregisterRange.id, unregisterRange.offset, unregisterRange.size, mode))
			status = -1;

	//send response
	connection->sendResponse(IOC_LF_MSG_OBJ_RANGE_UNREGISTER_ACK, request.lfClientId, status);

	//republish
	connection->repostReceive(request.msgBufferId);

	//
	return LF_WAIT_LOOP_KEEP_WAITING;
}
