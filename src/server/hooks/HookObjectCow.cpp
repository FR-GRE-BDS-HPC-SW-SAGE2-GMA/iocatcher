/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "base/common/Debug.hpp"
#include "base/network/LibfabricConnection.hpp"
#include "HookObjectCow.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the object create hook.
 * @param container The container to be able to access objects to create.
**/
HookObjectCow::HookObjectCow(Container * container)
{
	this->container = container;
}

/****************************************************/
LibfabricActionResult HookObjectCow::onMessage(LibfabricConnection * connection, int lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage)
{
	//debug
	IOC_DEBUG_ARG("hook:obj:cow", "Get copy on write from %1 to %2 from client %3")
		.arg(clientMessage->data.objCow.sourceObjectId)
		.arg(clientMessage->data.objCow.destObjectId)
		.arg(lfClientId)
		.end();
	
	//extract id
	LibfabricObjectId & sourceId = clientMessage->data.objCow.sourceObjectId;
	LibfabricObjectId & destId = clientMessage->data.objCow.destObjectId;;

	//create object
	bool status = this->container->makeObjectFullCow(sourceId, destId, clientMessage->data.objCow.allowExist);

	//fill response
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.type = IOC_LF_MSG_OBJ_COW_ACK;
	msg->header.clientId = lfClientId;
	msg->data.response.status = (status)?0:-1;

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
