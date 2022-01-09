/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
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
	LibfabricObjFlushInfos objFlush;
	request.deserializer.apply("objFlush", objFlush);

	//debug
	IOC_DEBUG_ARG("hook:obj:flush", "Get flush %1 client %2")
		.arg(Serializer::stringify(objFlush))
		.arg(request.lfClientId)
		.end();

	//flush object
	Object & object = this->container->getObject(objFlush.objectId);
	int ret = object.flush(objFlush.offset, objFlush.size);

	//send response
	connection->sendResponse(IOC_LF_MSG_OBJ_FLUSH_ACK, request.lfClientId, ret);

	//republish
	request.terminate();

	return LF_WAIT_LOOP_KEEP_WAITING;
}
