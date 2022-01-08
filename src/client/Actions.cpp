/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <cstring>
#include "Actions.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Implement the ping pong operation for the client side. We can ask to make the loop
 * as many time as wanted.
 * @param domain Reference to the libfabric domain to be used.
 * @param connection Reference to the client connection to be used.
 * @param cnt Number of time to make the ping pong roundtrip.
**/
void IOC::ping_pong(LibfabricDomain & domain, LibfabricConnection &connection, int cnt)
{
	//rma
	char * buffer = new char[TEST_RDMA_SIZE];
	memset(buffer, 0, TEST_RDMA_SIZE);
	Iov iov = domain.registerSegment(buffer, TEST_RDMA_SIZE, true, true, false);

	//send open
	LibfabricMessage msg;
	memset(&msg, 0, sizeof(msg));
	connection.fillProtocolHeader(msg.header, IOC_LF_MSG_PING);
	msg.data.iov = iov;

	//register hook
	connection.registerHook(IOC_LF_MSG_PONG, [](LibfabricConnection * connection, LibfabricClientRequest & request) {
		//printf("get 11 %d\n", clientId);
		connection->repostReceive(request.msgBufferId);
		return LF_WAIT_LOOP_UNBLOCK;
	});

	//time
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//send
	for (int i = 0 ; i < cnt ; i++) {
		//send message
		connection.sendMessageNoPollWakeup(&msg, sizeof (msg), IOC_LF_SERVER_ID);

		//poll server response
		LibfabricRemoteResponse serverResponse;
		bool hasMessage = connection.pollMessage(serverResponse, IOC_LF_MSG_PONG);
		assume(hasMessage, "Fail to get message from pollMessage !");

		///repost message
		connection.repostReceive(serverResponse);
	}

	//time
	clock_gettime(CLOCK_MONOTONIC, &stop);
	double result = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1e9;    // in microseconds
	double rate = (double)cnt / result / 1000.0;
	double bandwidth = 8.0 * (double)cnt * (double)(TEST_RDMA_SIZE+sizeof(msg)) / result / 1000.0 / 1000.0 / 1000.0;
	printf("Time: %g s, rate: %g kOPS, bandwidth: %g GBits/s, size: %zu\n", result, rate, bandwidth, sizeof(msg));

	//unregister
	domain.unregisterSegment(buffer, TEST_RDMA_SIZE);
}

/****************************************************/
/**
 * Make read read operation to read an object from the server. It can handle it via eager or RDMA operation.
 * @param connection Reference to the libfabric connection to use.
 * @param objectID The ID of the object to read.
 * @param buffer Buffer where to place the data. It will be automatically registered for RDMA operation and de-registered after the operation.
 * @param size Size of the read operation.
 * @param offset Offset of the data to read from the object.
 * @return Size of the read operation or negative value on error.
**/
ssize_t IOC::obj_read(LibfabricConnection &connection, const LibfabricObjectId & objectId, void* buffer, size_t size, size_t offset)
{
	//setup message request
	LibfabricMessage * msg = (LibfabricMessage *)connection.getDomain().getMsgBuffer();
	memset(msg, 0, sizeof(*msg));
	connection.fillProtocolHeader(msg->header, IOC_LF_MSG_OBJ_READ);
	msg->data.objReadWrite.objectId = objectId;
	msg->data.objReadWrite.offset = offset;
	msg->data.objReadWrite.size = size;
	msg->data.objReadWrite.msgHasData = false;

	//if rdma
	if (size > IOC_EAGER_MAX_READ) {
		//register
		Iov iov = connection.getDomain().registerSegment(buffer, size, true, true, false);
		msg->data.objReadWrite.iov = iov;
	}

	//send message
	connection.sendMessageNoPollWakeup(msg, sizeof (*msg), IOC_LF_SERVER_ID);

	//poll server response
	LibfabricRemoteResponse serverResponse;
	bool hasMessage = connection.pollMessage(serverResponse, IOC_LF_MSG_OBJ_READ_WRITE_ACK);
	assume(hasMessage, "Fail to get message from pollMessage !");

	//get status
	int status = serverResponse.message->data.response.status;

	//if need to copy data from eager exchange
	if (serverResponse.message->data.response.msgHasData) {
		assumeArg(serverResponse.message->data.response.msgDataSize == size, "Invalid message size recieved, expect %1 got %2")
			.arg(serverResponse.message->data.response.msgDataSize)
			.arg(size)
			.end();
		memcpy(buffer, serverResponse.message->extraData, serverResponse.message->data.response.msgDataSize);
	}

	//repost recv buffer
	connection.repostReceive(serverResponse);
	connection.getDomain().retMsgBuffer(msg);

	//unregister
	if (size > IOC_EAGER_MAX_READ)
		connection.getDomain().unregisterSegment(buffer, size);

	//check status
	if (status != 0)
		printf("Invalid status read : %d\n", status);
	
	//ret
	return status;
}

/****************************************************/
/**
 * Make read write operation to write to an object on the server. It can handle it via eager or RDMA operation.
 * @param connection Reference to the libfabric connection to use.
 * @param objectId The ID of the object to write.
 * @param buffer Buffer where to place the data. It will be automatically registered for RDMA operation and de-registered after the operation.
 * @param size Size of the write operation.
 * @param offset Offset of the data where to write in the object.
 * @return Size of the write operation or negative value on error.
**/
ssize_t IOC::obj_write(LibfabricConnection &connection, const LibfabricObjectId & objectId, const void* buffer, size_t size, size_t offset)
{
	//setup message request
	LibfabricMessage * msg = (LibfabricMessage *)connection.getDomain().getMsgBuffer();
	memset(msg, 0, sizeof(*msg));
	connection.fillProtocolHeader(msg->header, IOC_LF_MSG_OBJ_WRITE);
	msg->data.objReadWrite.objectId = objectId;
	msg->data.objReadWrite.offset = offset;
	msg->data.objReadWrite.size = size;
	msg->data.objReadWrite.msgHasData = false;

	//if rdma
	if (size > IOC_EAGER_MAX_WRITE)
	{
		//register
		Iov iov = connection.getDomain().registerSegment((char*)buffer, size, true, false, false);
		msg->data.objReadWrite.iov = iov;
	}

	//embed small data in message
	size_t toSend = sizeof(*msg);
	if (size <= IOC_EAGER_MAX_WRITE) {
		memcpy(msg->extraData, buffer, size);
		msg->data.objReadWrite.msgHasData = true;
		toSend += size;
	}

	//send message
	connection.sendMessageNoPollWakeup(msg, toSend, IOC_LF_SERVER_ID);

	//poll server response
	LibfabricRemoteResponse serverResponse;
	bool hasMessage = connection.pollMessage(serverResponse, IOC_LF_MSG_OBJ_READ_WRITE_ACK);
	assume(hasMessage, "Fail to get message from pollMessage !");

	//extract status and repost buffers
	int status = serverResponse.message->data.response.status;
	connection.repostReceive(serverResponse);
	connection.getDomain().retMsgBuffer(msg);

	//unregister
	if (size > IOC_EAGER_MAX_WRITE)
		connection.getDomain().unregisterSegment((char*)buffer, size);

	//check status
	if (status != 0)
		printf("Invalid status write : %d\n", status);
	
	return status;
}

/****************************************************/
/**
 * Perform a flush operation on a range space of the given object.
 * @param connection Reference to the libfabric connection to use.
 * @param objectID The ID of the object to flush.
 * @param offset Offset of the flush operation.
 * @param size Size of the segment to flush. Can use 0 to say all.
 * @return Return 0 on success, negative value on error.
**/
int IOC::obj_flush(LibfabricConnection &connection, const LibfabricObjectId & objectId, size_t offset, size_t size)
{
	//build message
	LibfabricObjFlushInfos objFlush = {
		.objectId = objectId,
		.offset = offset,
		.size = size,
	};

	//send message
	connection.sendMessageNoPollWakeup(IOC_LF_MSG_OBJ_FLUSH, IOC_LF_SERVER_ID, objFlush);

	//poll
	LibfabricRemoteResponse serverResponse;
	bool hasMessage = connection.pollMessage(serverResponse, IOC_LF_MSG_OBJ_FLUSH_ACK);
	assume(hasMessage, "Fail to get message from pollMessage !");

	//extract status & repost buffer
	LibfabricResponse response;
	serverResponse.deserializer.apply("response", response);
	int status = response.status;
	connection.repostReceive(serverResponse);

	//check status
	if (status != 0)
		printf("Invalid status flush : %d\n", status);
	
	return status;
}

/****************************************************/
/**
 * Perform a range registrion to notify we make a mapping on this part of the object.
 * This is used to prevent coherency issue by mapping in write mode several time the
 * same object.
 * @param connection Reference to the libfabric connection to use.
 * @param objectId The ID of the object in which to register segment.
 * @param offset Base offset of the mapping range to register.
 * @param size Size of the mapping range to register.
 * @param write Say if it read or read-write mapping.
 * @return An identifier to be used for de-registration to identify the mapping. It return negative value on error.
**/
int32_t IOC::obj_range_register(LibfabricConnection &connection, const LibfabricObjectId & objectId, size_t offset, size_t size, bool write)
{
	//setup message request
	LibfabricMessage msg;
	memset(&msg, 0, sizeof(msg));
	connection.fillProtocolHeader(msg.header, IOC_LF_MSG_OBJ_RANGE_REGISTER);
	msg.data.registerRange.objectId = objectId;
	msg.data.registerRange.offset = offset;
	msg.data.registerRange.size = size;
	msg.data.registerRange.write = write;

	//send message
	connection.sendMessageNoPollWakeup(&msg, sizeof (msg), IOC_LF_SERVER_ID);

	//poll
	LibfabricRemoteResponse serverResponse;
	bool hasMessage = connection.pollMessage(serverResponse, IOC_LF_MSG_OBJ_RANGE_REGISTER_ACK);
	assume(hasMessage, "Fail to get message from pollMessage !");

	//extract status
	int status = serverResponse.message->data.response.status;
	connection.repostReceive(serverResponse);

	//check status
	//if (status != 0)
	//	printf("Invalid status create : %d\n", status);
	
	return status;
}

/****************************************************/
/**
 * Deregister a mapping range to we can reuse it for other mappings.
 * @param connection Reference to the libfabric connection to use.
 * @param id If of the range to de-register.
 * @param objectId The ID of the object in which de-registering the range.
 * @param offset Base offset of the mapping range to register.
 * @param size Size of the mapping range to register.
 * @param write Say if it read or read-write mapping.
 * @return Return 0 on success and negative value on error.
**/
int IOC::obj_range_unregister(LibfabricConnection &connection, int32_t id, const LibfabricObjectId & objectId, size_t offset, size_t size, bool write)
{
	//setup message request
	LibfabricMessage msg;
	memset(&msg, 0, sizeof(msg));
	connection.fillProtocolHeader(msg.header, IOC_LF_MSG_OBJ_RANGE_UNREGISTER);
	msg.data.unregisterRange.id = id;
	msg.data.unregisterRange.objectId = objectId;
	msg.data.unregisterRange.offset = offset;
	msg.data.unregisterRange.size = size;
	msg.data.unregisterRange.write = write;

	//send message
	connection.sendMessageNoPollWakeup(&msg, sizeof (msg), IOC_LF_SERVER_ID);

	//poll
	LibfabricRemoteResponse serverResponse;
	bool hasMessage = connection.pollMessage(serverResponse, IOC_LF_MSG_OBJ_RANGE_UNREGISTER_ACK);
	assume(hasMessage, "Fail to get message from pollMessage !");

	//extract status & repost buffer
	int status = serverResponse.message->data.response.status;
	connection.repostReceive(serverResponse);

	//check status
	//if (status != 0)
	//	printf("Invalid status create : %d\n", status);
	
	return status;
}

/****************************************************/
/**
 * Create an object, this is mostly use to create an object in Mero before starting
 * to flush data to it otherwise it fails.
 * @param connection Reference to the libfabric connection to use.
 * @param objectID The ID of the object to create.
**/
int IOC::obj_create(LibfabricConnection &connection, const LibfabricObjectId & objectId)
{
	//setup message request
	LibfabricMessage msg;
	memset(&msg, 0, sizeof(msg));
	connection.fillProtocolHeader(msg.header, IOC_LF_MSG_OBJ_CREATE);
	msg.data.objFlush.objectId = objectId;

	//send message
	connection.sendMessageNoPollWakeup(&msg, sizeof (msg), IOC_LF_SERVER_ID);

	//poll
	LibfabricRemoteResponse serverResponse;
	bool hasMessage = connection.pollMessage(serverResponse, IOC_LF_MSG_OBJ_CREATE_ACK);
	assume(hasMessage, "Fail to get message from pollMessage !");

	//extract status & report buffer
	int status = serverResponse.message->data.response.status;
	connection.repostReceive(serverResponse);

	//check status
	//if (status != 0)
	//	printf("Invalid status create : %d\n", status);
	
	return status;
}

/****************************************************/
/**
 * Make a copy on write of an object.
 * @param connection Reference to the libfabric connection to use.
 * @param sourceObjectId ID of the source object.
 * @param destObjectId ID of the object to create.
 * @param allowExist Allow if object already exist.
 * @param offset The offset from which to cow.
 * @param size The size of the range to cow. 0 will reset the object and copy the full original object.
**/
int IOC::obj_cow(LibfabricConnection &connection, const LibfabricObjectId & sourceObjectId, const LibfabricObjectId & destObjectId, bool allowExist, size_t offset, size_t size)
{
	//setup message request
	LibfabricMessage msg;
	memset(&msg, 0, sizeof(msg));
	connection.fillProtocolHeader(msg.header, IOC_LF_MSG_OBJ_COW);
	msg.data.objCow.sourceObjectId = sourceObjectId;
	msg.data.objCow.destObjectId = destObjectId;
	msg.data.objCow.allowExist = allowExist;
	msg.data.objCow.rangeOffset = offset;
	msg.data.objCow.rangeSize = size;

	//send message
	connection.sendMessageNoPollWakeup(&msg, sizeof (msg), IOC_LF_SERVER_ID);

	//poll
	LibfabricRemoteResponse serverResponse;
	bool hasMessage = connection.pollMessage(serverResponse, IOC_LF_MSG_OBJ_COW_ACK);
	assume(hasMessage, "Fail to get message from pollMessage !");

	//extrace status & repost message
	int status = serverResponse.message->data.response.status;
	connection.repostReceive(serverResponse);

	//check status
	//if (status != 0)
	//	printf("Invalid status create : %d\n", status);
	
	return status;
}
