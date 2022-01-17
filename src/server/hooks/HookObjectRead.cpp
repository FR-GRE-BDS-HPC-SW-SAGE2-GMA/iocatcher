/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
//std
#include <cstring>
#include <cassert>
//local
#include "base/common/Debug.hpp"
#include "base/network/LibfabricConnection.hpp"
#include "HookObjectRead.hpp"
#include "../core/Consts.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the object read hook.
 * @param container The container to be able to access objects to with read operation.
**/
HookObjectRead::HookObjectRead(Container * container, ServerStats * stats)
{
	//check
	assert(container != NULL);
	assert(stats != NULL);

	//assign
	this->container = container;
	this->stats = stats;
}

/****************************************************/
/**
 * Send an error answer due to read failure.
 * @param connection Connection handler.
 * @param clientId Define the libfabric client ID.
 * @param clientMessage Pointer the the message requesting the RDMA read operation.
 * @param segments The list of object segments to transfer.
**/
void HookObjectRead::respondError(LibfabricConnection * connection, uint64_t clientId, LibfabricMessage * clientMessage)
{
	//send open
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.msgType = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
	msg->header.lfClientId = 0;
	msg->data.response.msgHasData = false;
	msg->data.response.msgDataSize = 0;
	msg->data.response.status = -1;

	//send ack message
	connection->sendMessage(msg, sizeof (*msg), clientId, [msg](void){
		delete msg;
		return LF_WAIT_LOOP_KEEP_WAITING;
	});
}

/****************************************************/
/**
 * Push data to the client via RDMA.
 * @param clientId Define the libfabric client ID.
 * @param clientMessage Pointer the the message requesting the RDMA read operation.
 * @param segments The list of object segments to transfer.
**/
void HookObjectRead::objRdmaPushToClient(LibfabricConnection * connection, uint64_t clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//build iovec
	iovec * iov = Object::buildIovec(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	size_t size = clientMessage->data.objReadWrite.size;

	//dispaly
	//printf("IOV SIZE %lu (%zu, %zu)\n", segments.size(), clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	//int cnt = 0;
	//for (auto it : segments)
	//	printf("-> %p %zu %zu (%p %zu)\n", it.ptr, it.offset, it.size, iov[cnt].iov_base, iov[cnt++].iov_len);

	//count number of ops
	int  * ops = new int;
	*ops = 0;
	for (size_t i = 0 ; i < segments.size() ; i += IOC_LF_MAX_RDMA_SEGS)
		(*ops)++;

	//loop on all send groups (because LF cannot send more than 256 at same time)
	size_t offset = 0;
	for (size_t i = 0 ; i < segments.size() ; i += IOC_LF_MAX_RDMA_SEGS) {
		//calc cnt
		size_t cnt = segments.size() - i;
		if (cnt > IOC_LF_MAX_RDMA_SEGS)
			cnt = IOC_LF_MAX_RDMA_SEGS;

		//extract
		char * addr = (char*)clientMessage->data.objReadWrite.iov.addr + offset;
		uint64_t key = clientMessage->data.objReadWrite.iov.key;

		//emit rdma write vec & implement callback
		connection->rdmaWritev(clientId, iov + i, cnt, addr, key, [this, connection, ops,size, clientId](void){
			//decrement
			(*ops)--;

			if (*ops == 0) {
				//send open
				LibfabricMessage * msg = new LibfabricMessage;
				msg->header.msgType = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
				msg->header.lfClientId = 0;
				msg->data.response.msgHasData = false;
				msg->data.response.msgDataSize = 0;
				msg->data.response.status = 0;

				//stats
				this->stats->readSize += size;

				//send ack message
				connection->sendMessage(msg, sizeof (*msg), clientId, [msg](void){
					delete msg;
					return LF_WAIT_LOOP_KEEP_WAITING;
				});

				//clean
				delete ops;
			}
			
			return LF_WAIT_LOOP_KEEP_WAITING;
		});

		//update offset
		for (size_t j = 0 ; j < cnt ; j++)
			offset += iov[i+j].iov_len;
	}

	//remove temp
	delete [] iov;
}

/****************************************************/
/**
 * Push data to the client making an eager communication and adding the data after the response
 * to the client.
 * @param clientId the libfabric client ID to know the connection to be used.
 * @param clientMessage the request from the client to get the required informations.
 * @param segments The list of object segments to be sent.
**/
void HookObjectRead::objEagerPushToClient(LibfabricConnection * connection, uint64_t clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//size
	size_t dataSize = clientMessage->data.objReadWrite.size;
	size_t baseOffset = clientMessage->data.objReadWrite.offset;

	//send open
	//char * buffer = new char[sizeof(LibfabricMessage) + dataSize];
	LibfabricMessage * msg = (LibfabricMessage *)connection->getDomain().getMsgBuffer();
	//LibfabricMessage * msg = (LibfabricMessage*)buffer;
	msg->header.msgType = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
	msg->header.lfClientId = 0;
	msg->data.response.msgDataSize = dataSize;
	msg->data.response.msgHasData = true;
	msg->data.response.status = 0;

	//get base pointer
	char * data = msg->extraData;

	//copy data
	size_t cur = 0;
	for (auto segment : segments) {
		//compute copy size to stay in data limits
		size_t copySize = segment.size;
		size_t offset = 0;
		if (baseOffset > segment.offset) {
			offset = baseOffset - segment.offset;
			copySize -= offset;
		}
		assert(copySize <= segment.size);
		if (cur + copySize > dataSize) {
			copySize = dataSize - cur;
		}

		//copy
		assert(offset < segment.size);
		assert(copySize <= segment.size);
		assert(copySize <= segment.size - offset);
		memcpy(data + cur, segment.ptr + offset, copySize);

		//progress
		cur += copySize;
	}

	//stats
	this->stats->readSize += dataSize;

	//send ack message
	connection->sendMessage(msg, sizeof (*msg) + dataSize, clientId, [connection, msg](void){
		connection->getDomain().retMsgBuffer(msg);
		return LF_WAIT_LOOP_KEEP_WAITING;
	});
}

/****************************************************/
LibfabricActionResult HookObjectRead::onMessage(LibfabricConnection * connection, uint64_t lfClientId, size_t msgBufferId, LibfabricMessage * clientMessage)
{
	//debug
	IOC_DEBUG_ARG("hook:obj:read", "Get object read on %1 for %2->%3 from client %4")
		.arg(clientMessage->data.objReadWrite.objectId)
		.arg(clientMessage->data.objReadWrite.offset)
		.arg(clientMessage->data.objReadWrite.size)
		.arg(lfClientId)
		.end();

	//get buffers from object
	Object & object = this->container->getObject(clientMessage->data.objReadWrite.objectId);
	ObjectSegmentList segments;
	bool status = object.getBuffers(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size, ACCESS_READ);

	//eager or rdma
	if (status) {
		if (clientMessage->data.objReadWrite.size <= IOC_EAGER_MAX_READ) {
			this->objEagerPushToClient(connection, lfClientId, clientMessage, segments);
		} else {
			this->objRdmaPushToClient(connection, lfClientId, clientMessage, segments);
		}
	} else {
		this->respondError(connection, lfClientId, clientMessage);
	}

	//republish
	connection->repostRecive(msgBufferId);

	return LF_WAIT_LOOP_KEEP_WAITING;
}
