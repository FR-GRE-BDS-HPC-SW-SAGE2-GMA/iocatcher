/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "base/network/LibfabricConnection.hpp"
#include "HookPingPong.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
LibfabricActionResult HookPingPong::onMessage(LibfabricConnection * connection, int lfClientId, size_t msgBufferId, LibfabricMessage * message)
{
	//prepare answer
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.type = IOC_LF_MSG_PONG;
	msg->header.clientId = 0;

	//send message
	connection->sendMessage(msg, sizeof (*msg), lfClientId, [msg](void){
		delete msg;
		return LF_WAIT_LOOP_KEEP_WAITING;
	});

	//republish
	connection->repostRecive(msgBufferId);

	return LF_WAIT_LOOP_KEEP_WAITING;
}
