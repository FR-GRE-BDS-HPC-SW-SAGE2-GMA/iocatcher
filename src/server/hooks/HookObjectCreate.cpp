/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "base/network/LibfabricConnection.hpp"
#include "HookObjectCreate.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the object create hook.
 * @param container The container to be able to access objects to create.
**/
HookObjectCreate::HookObjectCreate(Container * container)
{
	this->container = container;
}

/****************************************************/
LibfabricActionResult HookObjectCreate::onMessage(LibfabricConnection * connection, int lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage)
{
	//printf
	//printf("Get create object %ld:%ld\n", clientMessage->data.objCreate.high, clientMessage->data.objCreate.low);

	//create object
	Object & object = this->container->getObject(clientMessage->data.objFlush.high, clientMessage->data.objFlush.low);
	int ret = object.create();

	//fill response
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.type = IOC_LF_MSG_OBJ_CREATE_ACK;
	msg->header.clientId = lfClientId;
	msg->data.response.status = ret;

	//send message
	connection->sendMessage(msg, sizeof (*msg), lfClientId, [msg](void){
		delete msg;
		return LF_WAIT_LOOP_KEEP_WAITING;
	});

	//republish
	connection->repostRecive(msgBufferId);

	//ret
	return LF_WAIT_LOOP_KEEP_WAITING;
}
