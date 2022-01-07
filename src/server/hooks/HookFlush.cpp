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
LibfabricActionResult HookFlush::onMessage(LibfabricConnection * connection, LibfabricClientRequest & request)
{
	//extract
	LibfabricObjFlushInfos & objFlush = request.message->data.objFlush;

	//debug
	IOC_DEBUG_ARG("hook:obj:flush", "Get flush object %1 on %2->%3 from client %4")
		.arg(objFlush.objectId)
		.arg(objFlush.offset)
		.arg(objFlush.size)
		.arg(request.lfClientId)
		.end();

	//flush object
	Object & object = this->container->getObject(request.message->data.objFlush.objectId);
	int ret = object.flush(request.message->data.objFlush.offset, objFlush.size);

	//send response
	connection->sendResponse(IOC_LF_MSG_OBJ_FLUSH_ACK, request.lfClientId, ret);

	//republish
	connection->repostReceive(request.msgBufferId);

	return LF_WAIT_LOOP_KEEP_WAITING;
}
