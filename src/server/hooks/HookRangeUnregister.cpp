/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
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
LibfabricActionResult HookRangeUnregister::onMessage(LibfabricConnection * connection, int lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage)
{
	//get object
	Object & object = this->container->getObject(clientMessage->data.registerRange.high, clientMessage->data.registerRange.low);
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

	//return message
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.type = IOC_LF_MSG_OBJ_RANGE_UNREGISTER_ACK;
	msg->header.clientId = lfClientId;
	msg->data.response.status = status;
	msg->data.response.msgHasData = false;

	connection->sendMessage(msg, sizeof (*msg), lfClientId, [msg](){
		delete msg;
		return LF_WAIT_LOOP_KEEP_WAITING;
	});

	//republish
	connection->repostRecive(msgBufferId);

	//
	return LF_WAIT_LOOP_KEEP_WAITING;
}
