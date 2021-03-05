/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
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
LibfabricActionResult HookRangeRegister::onMessage(LibfabricConnection * connection, int lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage)
{
	//get object
	Object & object = this->container->getObject(clientMessage->data.registerRange.high, clientMessage->data.registerRange.low);
	ConsistencyTracker & tracker = object.getConsistencyTracker();

	//check
	int status = 0;
	ConsistencyAccessMode mode = CONSIST_ACCESS_MODE_READ;
	if (clientMessage->data.registerRange.write)
		mode = CONSIST_ACCESS_MODE_WRITE;
	if (this->config->consistencyCheck)
		status = tracker.registerRange(clientMessage->header.tcpClientId, clientMessage->data.registerRange.offset, clientMessage->data.registerRange.size, mode);

	//return message
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.type = IOC_LF_MSG_OBJ_RANGE_REGISTER_ACK;
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
