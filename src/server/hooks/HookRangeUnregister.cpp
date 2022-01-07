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
LibfabricActionResult HookRangeUnregister::onMessage(LibfabricConnection * connection, uint64_t lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage)
{
	//debug
	IOC_DEBUG_ARG("hook:range:unregister", "Get range unregister %1 on object %2 (%3->%4) from client %5")
		.arg(clientMessage->data.unregisterRange.id)
		.arg(clientMessage->data.registerRange.objectId)
		.arg(clientMessage->data.registerRange.offset)
		.arg(clientMessage->data.registerRange.size)
		.arg(lfClientId)
		.end();

	//get object
	Object & object = this->container->getObject(clientMessage->data.registerRange.objectId);
	ConsistencyTracker & tracker = object.getConsistencyTracker();

	//extract
	LibfabricUnregisterRange &data = clientMessage->data.unregisterRange;

	//check
	int status = 0;
	ConsistencyAccessMode mode = CONSIST_ACCESS_MODE_READ;
	if (data.write)
		mode = CONSIST_ACCESS_MODE_WRITE;
	if (this->config->consistencyCheck)
		if (!tracker.unregisterRange(clientMessage->header.tcpClientId, data.id, data.offset, data.size, mode))
			status = -1;

	//send response
	connection->sendResponse(IOC_LF_MSG_OBJ_RANGE_UNREGISTER_ACK, lfClientId, status);

	//republish
	connection->repostReceive(msgBufferId);

	//
	return LF_WAIT_LOOP_KEEP_WAITING;
}
