/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "base/common/Debug.hpp"
#include "base/network/LibfabricConnection.hpp"
#include "HookRangeRegister.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the range register hook.
 * @param config The config object to know if range tracking is enabled.
 * @param container The container to be able to access objects to register ranges.
**/
HookRangeRegister::HookRangeRegister(const Config * config, Container * container)
{
	this->config = config;
	this->container = container;
}

/****************************************************/
LibfabricActionResult HookRangeRegister::onMessage(LibfabricConnection * connection, uint64_t lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage)
{
	//get object
	Object & object = this->container->getObject(clientMessage->data.registerRange.objectId);
	ConsistencyTracker & tracker = object.getConsistencyTracker();

	//check
	int status = 0;
	ConsistencyAccessMode mode = CONSIST_ACCESS_MODE_READ;
	if (clientMessage->data.registerRange.write)
		mode = CONSIST_ACCESS_MODE_WRITE;
	if (this->config->consistencyCheck)
		status = tracker.registerRange(clientMessage->header.tcpClientId, clientMessage->data.registerRange.offset, clientMessage->data.registerRange.size, mode);
	
	//debug
	IOC_DEBUG_ARG("hook:range:register", "Get range register on object %1 (%2->%3) from client %4, response=%5")
		.arg(clientMessage->data.registerRange.objectId)
		.arg(clientMessage->data.registerRange.offset)
		.arg(clientMessage->data.registerRange.size)
		.arg(lfClientId)
		.arg(status)
		.end();

	//send response
	connection->sendReponse(IOC_LF_MSG_OBJ_RANGE_REGISTER_ACK, lfClientId, status);

	//republish
	connection->repostRecive(msgBufferId);

	//
	return LF_WAIT_LOOP_KEEP_WAITING;
}
