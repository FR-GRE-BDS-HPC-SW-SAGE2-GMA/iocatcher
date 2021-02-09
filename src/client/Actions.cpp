/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "Actions.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
void IOC::ping_pong(LibfabricDomain & domain, LibfabricConnection &connection)
{
	//rma
	char * buffer = new char[TEST_RDMA_SIZE];
	memset(buffer, 0, TEST_RDMA_SIZE);
	Iov iov = domain.registerSegment(buffer, TEST_RDMA_SIZE, true, true, false);

	//send open
	LibfabricMessage msg;
	msg.header.type = IOC_LF_MSG_PING;
	msg.header.clientId = connection.getClientId();
	msg.data.iov = iov;

	//register hook
	connection.registerHook(IOC_LF_MSG_PONG, [&connection](int clientId, size_t id, void * buffer) {
		//printf("get 11 %d\n", clientId);
		connection.repostRecive(id);
		return true;
	});

	//time
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//send
	int cnt = 100000;
	for (int i = 0 ; i < cnt ; i++) {
		connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, [msg](LibfabricPostAction*action){
			return false;
		});

		//poll
		connection.poll(true);
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
ssize_t IOC::obj_read(LibfabricConnection &connection, int64_t high, int64_t low, void* buffer, size_t size, size_t offset)
{
	//setup message request
	LibfabricMessage * msg = (LibfabricMessage *)connection.getDomain().getMsgBuffer();
	msg->header.type = IOC_LF_MSG_OBJ_READ;
	msg->header.clientId = connection.getClientId();
	msg->data.objReadWrite.low = low;
	msg->data.objReadWrite.high = high;
	msg->data.objReadWrite.offset = offset;
	msg->data.objReadWrite.size = size;
	msg->data.objReadWrite.msgHasData = false;

	//if rdma
	if (size > IOC_EAGER_MAX_READ) {
		//register
		Iov iov = connection.getDomain().registerSegment(buffer, size, true, true, false);
		msg->data.objReadWrite.iov = iov;
	}

	//register hook for reception
	int status = -1;
	connection.registerHook(IOC_LF_MSG_OBJ_READ_WRITE_ACK, [&connection, &status, buffer, size](int clientId, size_t id, void * msgBuffer) {
		//extract status
		LibfabricMessage * ackMsg = (LibfabricMessage *)msgBuffer;
		status = ackMsg->data.response.status;

		//if need to copy data from eager exchange
		if (ackMsg->data.response.msgHasData) {
			assumeArg(ackMsg->data.response.msgDataSize == size, "Invalid message size recieved, expect %1 got %2")
				.arg(ackMsg->data.response.msgDataSize)
				.arg(size)
				.end();
			memcpy(buffer, ackMsg + 1, ackMsg->data.response.msgDataSize);
		}

		//repost recv buffer
		connection.repostRecive(id);
		return true;
	});

	//send message
	connection.sendMessage(msg, sizeof (*msg), IOC_LF_SERVER_ID, [msg, &connection](LibfabricPostAction*action){
		connection.getDomain().retMsgBuffer(msg);
		return false;
	});

	//poll
	connection.poll(true);

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
ssize_t IOC::obj_write(LibfabricConnection &connection, int64_t high, int64_t low, const void* buffer, size_t size, size_t offset)
{
	//setup message request
	LibfabricMessage * msg = (LibfabricMessage *)connection.getDomain().getMsgBuffer();
	msg->header.type = IOC_LF_MSG_OBJ_WRITE;
	msg->header.clientId = connection.getClientId();
	msg->data.objReadWrite.low = low;
	msg->data.objReadWrite.high = high;
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
		memcpy(msg+1, buffer, size);
		msg->data.objReadWrite.msgHasData = true;
		toSend += size;
	}

	//register hook for reception
	LibfabricMessage ackMsg;
	connection.registerHook(IOC_LF_MSG_OBJ_READ_WRITE_ACK, [&connection,&ackMsg](int clientId, size_t id, void * buffer) {
		ackMsg = *(LibfabricMessage *)buffer;
		//printf("get 11 %d\n", clientId);
		connection.repostRecive(id);
		return true;
	});

	//send message
	connection.sendMessage(msg, toSend, IOC_LF_SERVER_ID, [msg, &connection](LibfabricPostAction*action){
		connection.getDomain().retMsgBuffer(msg);
		return false;
	});

	//poll
	connection.poll(true);

	//unregister
	if (size > IOC_EAGER_MAX_WRITE)
		connection.getDomain().unregisterSegment((char*)buffer, size);

	//check status
	if (ackMsg.data.response.status != 0)
		printf("Invalid status write : %d\n", ackMsg.data.response.status);
	
	return ackMsg.data.response.status;
}

/****************************************************/
int IOC::obj_flush(LibfabricConnection &connection, int64_t high, int64_t low, size_t offset, size_t size)
{
	//setup message request
	LibfabricMessage msg;
	msg.header.type = IOC_LF_MSG_OBJ_FLUSH;
	msg.header.clientId = connection.getClientId();
	msg.data.objFlush.low = low;
	msg.data.objFlush.high = high;
	msg.data.objFlush.offset = offset;
	msg.data.objFlush.size = size;

	//register hook for reception
	LibfabricMessage ackMsg;
	connection.registerHook(IOC_LF_MSG_OBJ_FLUSH_ACK, [&connection,&ackMsg](int clientId, size_t id, void * buffer) {
		ackMsg = *(LibfabricMessage *)buffer;
		//printf("get 11 %d\n", clientId);
		connection.repostRecive(id);
		return true;
	});

	//send message
	connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, [msg](LibfabricPostAction*action){
		return false;
	});

	//poll
	connection.poll(true);

	//check status
	if (ackMsg.data.response.status != 0)
		printf("Invalid status flush : %d\n", ackMsg.data.response.status);
	
	return ackMsg.data.response.status;
}

/****************************************************/
int IOC::obj_create(LibfabricConnection &connection, int64_t high, int64_t low)
{
	//setup message request
	LibfabricMessage msg;
	msg.header.type = IOC_LF_MSG_OBJ_CREATE;
	msg.header.clientId = connection.getClientId();
	msg.data.objFlush.low = low;
	msg.data.objFlush.high = high;

	//register hook for reception
	LibfabricMessage ackMsg;
	connection.registerHook(IOC_LF_MSG_OBJ_CREATE_ACK, [&connection,&ackMsg](int clientId, size_t id, void * buffer) {
		ackMsg = *(LibfabricMessage *)buffer;
		//printf("get 11 %d\n", clientId);
		connection.repostRecive(id);
		return true;
	});

	//send message
	connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, [msg](LibfabricPostAction*action){
		return false;
	});

	//poll
	connection.poll(true);

	//check status
	//if (ackMsg.data.response.status != 0)
	//	printf("Invalid status create : %d\n", ackMsg.data.response.status);
	
	return ackMsg.data.response.status;
}
