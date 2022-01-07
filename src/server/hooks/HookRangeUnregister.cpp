/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
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
LibfabricActionResult HookRangeUnregister::onMessage(LibfabricConnection * connection, LibfabricClientMessage & message)
{
	//extract
	LibfabricUnregisterRange & unregisterRange = message.message->data.unregisterRange;

	//debug
	IOC_DEBUG_ARG("hook:range:unregister", "Get range unregister %1 on object %2 (%3->%4) from client %5")
		.arg(unregisterRange.id)
		.arg(unregisterRange.objectId)
		.arg(unregisterRange.offset)
		.arg(unregisterRange.size)
		.arg(message.lfClientId)
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
		if (!tracker.unregisterRange(message.header->tcpClientId, unregisterRange.id, unregisterRange.offset, unregisterRange.size, mode))
			status = -1;

	//send response
	connection->sendResponse(IOC_LF_MSG_OBJ_RANGE_UNREGISTER_ACK, message.lfClientId, status);

	//republish
	connection->repostReceive(message.msgBufferId);

	//
	return LF_WAIT_LOOP_KEEP_WAITING;
}
