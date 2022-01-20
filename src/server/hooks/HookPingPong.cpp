/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include "base/common/Debug.hpp"
#include "base/network/LibfabricConnection.hpp"
#include "HookPingPong.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
HookPingPong::HookPingPong(LibfabricDomain * domain)
{
	assert(domain != NULL);
	this->buffer = NULL;
	this->bufferSize = 0;
	this->domain = domain;
	this->bufferIov.addr = 0;
	this->bufferIov.key = 0;
}

/****************************************************/
HookPingPong::~HookPingPong(void)
{
	this->reallocateBuffer(0);
}

/****************************************************/
void HookPingPong::reallocateBuffer(size_t rdmaSize)
{
	if (this->buffer != NULL) {
		domain->unregisterSegment(buffer, bufferSize);
		delete [] buffer;
	}
	if (rdmaSize > 0) {
		buffer = new char[rdmaSize];
		bufferSize = rdmaSize;
		bufferIov = domain->registerSegment(buffer, rdmaSize, true, true, false);
	} else {
		this->bufferSize = 0;
		this->buffer = NULL;
		this->bufferIov.addr = 0;
		this->bufferIov.key = 0;
	}
}

/****************************************************/
LibfabricActionResult HookPingPong::onMessage(LibfabricConnection * connection, LibfabricClientRequest & request)
{
	//extract
	LibfabricPing ping;
	request.deserializer.apply("ping", ping);

	//debug
	IOC_DEBUG_ARG("hook:ping", "Get ping message %1 from client %2")
		.arg(Serializer::stringify(ping))
		.arg(request.lfClientId)
		.end();
	
	//reallocate if needed
	if (this->bufferSize < ping.rdmaSize)
		this->reallocateBuffer(ping.rdmaSize);

	//check eager data
	for (size_t i = 0 ; i < ping.eagerSize ; i++)
		assert(ping.eagerData[i] == 43);

	//apply rdma
	uint64_t lfClientId = request.lfClientId;
	size_t msgBufferId = request.msgBufferId;
	if (ping.rdmaSize > 0) {
		connection->rdmaRead(request.lfClientId, this->buffer, ping.rdmaIov.addr, ping.rdmaIov.key, ping.rdmaSize, [this, connection, lfClientId, msgBufferId, ping](void) {
			//check eager data
			for (size_t i = 0 ; i < ping.rdmaSize ; i++)
				assert(this->buffer[i] == 42);

			//send response
			connection->sendResponse(IOC_LF_MSG_PONG, lfClientId, 0);

			//republish
			connection->repostReceive(msgBufferId);

			//keep waiting
			return LF_WAIT_LOOP_KEEP_WAITING;
		});
	} else {
		//send response
		connection->sendResponse(IOC_LF_MSG_PONG, lfClientId, 0);

		//republish
		connection->repostReceive(msgBufferId);
	}

	return LF_WAIT_LOOP_KEEP_WAITING;
}
