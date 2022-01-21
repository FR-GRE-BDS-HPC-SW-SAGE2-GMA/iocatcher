/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include "base/common/Debug.hpp"
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
LibfabricActionResult HookObjectCreate::onMessage(LibfabricConnection * connection, LibfabricClientRequest & request)
{
	//extract
	LibfabricObjCreateInfos objCreate;
	request.deserializer.apply("objCreate", objCreate);

	//debug
	IOC_DEBUG_ARG("hook:obj:create", "Get create object %1 from client %2")
		.arg(Serializer::stringify(objCreate))
		.arg(request.lfClientId)
		.end();

	//create object
	Object & object = this->container->getObject(objCreate.objectId);
	int ret = object.create();

	//send response
	connection->sendResponse(IOC_LF_MSG_OBJ_CREATE_ACK, request.lfClientId, ret);

	//republish
	request.terminate();

	//ret
	return LF_WAIT_LOOP_KEEP_WAITING;
}
