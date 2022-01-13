/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cstring>
#include <cassert>
//local
#include "base/common/Debug.hpp"
#include "base/network/LibfabricConnection.hpp"
#include "HookObjectWrite.hpp"
#include "../core/Consts.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Constructor of the object write hook.
 * @param container The container to be able to access objects to with write operation.
**/
HookObjectWrite::HookObjectWrite(Container * container, ServerStats * stats)
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
 * Fetch data from the client via RDMA.
 * @param clientId Define the libfabric client ID.
 * @param objReadWrite Reference to the write request information.
 * @param segments The list of object segments to transfer.
**/
void HookObjectWrite::objRdmaFetchFromClient(LibfabricConnection * connection, uint64_t clientId, LibfabricObjReadWriteInfos & objReadWrite, ObjectSegmentList & segments)
{
	//build iovec
	iovec * iov = Object::buildIovec(segments, objReadWrite.offset, objReadWrite.size);
	size_t size = objReadWrite.size;

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

		//emit rdma write vec & implement callback
		char * addr = (char*)objReadWrite.iov.addr + offset;
		uint64_t key = objReadWrite.iov.key;
		connection->rdmaReadv(clientId, iov + i, cnt, addr, key, [connection, ops, size, this, clientId](void){
			//decrement
			(*ops)--;

			if (*ops == 0) {
				//stats
				this->stats->writeSize += size;

				//send response
				connection->sendResponse(IOC_LF_MSG_OBJ_READ_WRITE_ACK, clientId, 0);

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
 * Pull data to the client getting an eager communication and copying it directly to the object segments.
 * to the client.
 * @param clientId the libfabric client ID to know the connection to be used.
 * @param clientMessage the request from the client to get the required informations.
 * @param segments The list of object segments to be sent.
**/
void HookObjectWrite::objEagerExtractFromMessage(LibfabricConnection * connection, uint64_t clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//get base pointer
	char * data = clientMessage->extraData;

	//copy data
	size_t cur = 0;
	size_t dataSize = clientMessage->data.objReadWrite.size;
	size_t baseOffset = clientMessage->data.objReadWrite.offset;
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
		memcpy(segment.ptr + offset, data + cur, copySize);

		//progress
		cur += copySize;
	}

	//stats
	this->stats->writeSize += cur;

	//send response
	connection->sendResponse(IOC_LF_MSG_OBJ_READ_WRITE_ACK, clientId, 0);
}

/****************************************************/
LibfabricActionResult HookObjectWrite::onMessage(LibfabricConnection * connection, LibfabricClientRequest & request)
{
	//extract
	LibfabricObjReadWriteInfos & objReadWrite = request.message->data.objReadWrite;

	//debug
	IOC_DEBUG_ARG("hook:obj:write", "Get object write on %1 for %2->%3 from client %4")
		.arg(objReadWrite.objectId)
		.arg(objReadWrite.offset)
		.arg(objReadWrite.size)
		.arg(request.lfClientId)
		.end();

	//get buffers from object
	Object & object = this->container->getObject(objReadWrite.objectId);
	ObjectSegmentList segments;
	bool status = object.getBuffers(segments, objReadWrite.offset, objReadWrite.size, ACCESS_WRITE, true, true);

	//eager or rdma
	if (status) {
		if (objReadWrite.msgHasData) {
			objEagerExtractFromMessage(connection, request.lfClientId, request.message, segments);
		} else {
			objRdmaFetchFromClient(connection, request.lfClientId, objReadWrite, segments);
		}
	} else {
		connection->sendResponse(IOC_LF_MSG_OBJ_READ_WRITE_ACK, request.lfClientId, 0);
	}

	//mark dirty
	object.markDirty(objReadWrite.offset, objReadWrite.size);

	//republish
	connection->repostReceive(request.msgBufferId);

	return LF_WAIT_LOOP_KEEP_WAITING;
}
