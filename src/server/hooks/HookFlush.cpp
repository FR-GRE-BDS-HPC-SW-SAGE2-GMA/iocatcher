/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "base/common/Debug.hpp"
#include "base/network/LibfabricConnection.hpp"
#include "HookFlush.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the flush hook.
 * @param container The container to be able to access objects to flush.
**/
HookFlush::HookFlush(Container * container)
{
	this->container = container;
}

/****************************************************/
LibfabricActionResult HookFlush::onMessage(LibfabricConnection * connection, int lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage)
{
	//debug
	IOC_DEBUG_ARG("hook:obj:flush", "Get flush object %1 on %2->%3 from client %4")
		.arg(clientMessage->data.objFlush.objectId)
		.arg(clientMessage->data.objFlush.offset)
		.arg(clientMessage->data.objFlush.size)
		.arg(lfClientId)
		.end();

	//flush object
	Object & object = this->container->getObject(clientMessage->data.objFlush.objectId);
	int ret = object.flush(clientMessage->data.objFlush.offset, clientMessage->data.objFlush.size);

	//prepare message
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.msgType = IOC_LF_MSG_OBJ_FLUSH_ACK;
	msg->header.clientId = lfClientId;
	msg->data.response.status = ret;

	//send message
	connection->sendMessage(msg, sizeof (*msg), lfClientId, [msg](void){
		delete msg;
		return LF_WAIT_LOOP_KEEP_WAITING;
	});

	//republish
	connection->repostRecive(msgBufferId);

	return LF_WAIT_LOOP_KEEP_WAITING;
}
