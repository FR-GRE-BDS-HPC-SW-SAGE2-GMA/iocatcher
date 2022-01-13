/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "base/common/Debug.hpp"
#include "base/network/LibfabricConnection.hpp"
#include "HookPingPong.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
LibfabricActionResult HookPingPong::onMessage(LibfabricConnection * connection, LibfabricClientRequest & request)
{
	//debug
	IOC_DEBUG_ARG("hook:ping", "Get ping message from client %1")
		.arg(request.lfClientId)
		.end();

	//prepare answer
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.msgType = IOC_LF_MSG_PONG;
	msg->header.lfClientId = 0;

	//send message
	connection->sendMessage(msg, sizeof (*msg), request.lfClientId, [msg](void){
		delete msg;
		return LF_WAIT_LOOP_KEEP_WAITING;
	});

	//republish
	connection->repostReceive(request.msgBufferId);

	return LF_WAIT_LOOP_KEEP_WAITING;
}
