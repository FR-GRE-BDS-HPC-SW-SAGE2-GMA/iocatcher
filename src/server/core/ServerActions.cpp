/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include "../../base/network/LibfabricConnection.hpp"
#include "ServerActions.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
//TODO make a global config object
bool IOC::gblConsistencyCheck = true;
size_t IOC::gblReadSize = 0;
size_t IOC::gblWriteSize = 0;

/****************************************************/
void IOC::setupPingPong(LibfabricConnection & connection)
{
	//rma
	char * rmaBuffer = new char[TEST_RDMA_SIZE];
	connection.getDomain().registerSegment(rmaBuffer, TEST_RDMA_SIZE, true, true, false);

	//register hook
	connection.registerHook(IOC_LF_MSG_PING, [&connection, rmaBuffer](int clientId, size_t id, void * buffer) {
		//printf
		//printf("Get 10 %d\n", clientId);

		//do rdma read
		//LibfabricMessage * clientMessage = (LibfabricMessage *)buffer;
		//connection.rdmaWrite(clientId, rmaBuffer, clientMessage->data.iov.addr, clientMessage->data.iov.key, TEST_RDMA_SIZE, new LibfabricPostActionFunction([&connection, clientId](void){
			//send open
			LibfabricMessage * msg = new LibfabricMessage;
			msg->header.type = IOC_LF_MSG_PONG;
			msg->header.clientId = 0;

			connection.sendMessage(msg, sizeof (*msg), clientId, [msg](void){
				delete msg;
				return false;
			});
			
		//	return false;
		//}));

		//republish
		connection.repostRecive(id);

		return false;
	});
}

/****************************************************/
void IOC::setupObjFlush(LibfabricConnection & connection, Container & container)
{
	//register hook
	connection.registerHook(IOC_LF_MSG_OBJ_FLUSH, [&connection, &container](int clientId, size_t id, void * buffer) {
		//infos
		LibfabricMessage * clientMessage = (LibfabricMessage*)buffer;

		//printf
		//printf("Get flush object %ld:%ld %lu->%lu\n", clientMessage->data.objFlush.high, clientMessage->data.objFlush.low, clientMessage->data.objFlush.offset, clientMessage->data.objFlush.size);

		//flush object
		Object & object = container.getObject(clientMessage->data.objFlush.low, clientMessage->data.objFlush.high);
		int ret = object.flush(clientMessage->data.objFlush.offset, clientMessage->data.objFlush.size);

		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_FLUSH_ACK;
		msg->header.clientId = clientId;
		msg->data.response.status = ret;

		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](void){
			delete msg;
			return false;
		});

		//republish
		connection.repostRecive(id);

		return false;
	});
}

/****************************************************/
void IOC::setupObjRangeRegister(LibfabricConnection & connection, Container & container)
{
	//register hook
	connection.registerHook(IOC_LF_MSG_OBJ_RANGE_REGISTER, [&connection, &container](int clientId, size_t id, void * buffer) {
		//infos
		LibfabricMessage * clientMessage = (LibfabricMessage*)buffer;

		//get object
		Object & object = container.getObject(clientMessage->data.registerRange.low, clientMessage->data.registerRange.high);
		ConsistencyTracker & tracker = object.getConsistencyTracker();

		//check
		int status = 0;
		ConsistencyAccessMode mode = CONSIST_ACCESS_MODE_READ;
		if (clientMessage->data.registerRange.write)
			mode = CONSIST_ACCESS_MODE_WRITE;
		if (gblConsistencyCheck)
			if (!tracker.registerRange(clientMessage->data.registerRange.offset, clientMessage->data.registerRange.size, mode))
				status = -1;

		//return message
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_RANGE_REGISTER_ACK;
		msg->header.clientId = clientId;
		msg->data.response.status = status;
		msg->data.response.msgHasData = false;

		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](){
			delete msg;
			return false;
		});

		//republish
		connection.repostRecive(id);

		//
		return false;
	});
}

/****************************************************/
void IOC::setupObjUnregisterRange(LibfabricConnection & connection, Container & container)
{
	//register hook
	connection.registerHook(IOC_LF_MSG_OBJ_RANGE_UNREGISTER, [&connection, &container](int clientId, size_t id, void * buffer) {
		//infos
		LibfabricMessage * clientMessage = (LibfabricMessage*)buffer;

		//get object
		Object & object = container.getObject(clientMessage->data.registerRange.low, clientMessage->data.registerRange.high);
		ConsistencyTracker & tracker = object.getConsistencyTracker();

		//check
		int status = 0;
		ConsistencyAccessMode mode = CONSIST_ACCESS_MODE_READ;
		if (clientMessage->data.registerRange.write)
			mode = CONSIST_ACCESS_MODE_WRITE;
		if (gblConsistencyCheck)
			if (!tracker.unregisterRange(clientMessage->data.registerRange.offset, clientMessage->data.registerRange.size))
				status = -1;

		//return message
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_RANGE_UNREGISTER_ACK;
		msg->header.clientId = clientId;
		msg->data.response.status = status;
		msg->data.response.msgHasData = false;

		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](){
			delete msg;
			return false;
		});

		//republish
		connection.repostRecive(id);

		//
		return false;
	});
}

/****************************************************/
void IOC::setupObjCreate(LibfabricConnection & connection, Container & container)
{
	//register hook
	connection.registerHook(IOC_LF_MSG_OBJ_CREATE, [&connection, &container](int clientId, size_t id, void * buffer) {
		//infos
		LibfabricMessage * clientMessage = (LibfabricMessage*)buffer;

		//printf
		printf("Get create object %ld:%ld\n", clientMessage->data.objCreate.high, clientMessage->data.objCreate.low);

		//create object
		Object & object = container.getObject(clientMessage->data.objFlush.low, clientMessage->data.objFlush.high);
		int ret = object.create();

		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_CREATE_ACK;
		msg->header.clientId = clientId;
		msg->data.response.status = ret;

		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](void){
			delete msg;
			return false;
		});

		//republish
		connection.repostRecive(id);

		return false;
	});
}

/****************************************************/
iovec * IOC::buildIovec(ObjectSegmentList & segments, size_t offset, size_t size)
{
	//compute intersection
	for (auto & it : segments) {
		if (it.offset < offset) {
			int delta = offset - it.offset;
			it.ptr += delta;
			it.offset += delta;
			it.size -= delta;
		}
		if (it.offset + it.size > offset + size) {
			int delta = it.offset + it.size - (offset + size);
			it.size -= delta;
		}
	}

	//build iov
	struct iovec * iov = new iovec[segments.size()];
	int cnt = 0;
	for (auto & it : segments) {
		iov[cnt].iov_base = it.ptr;
		iov[cnt].iov_len = it.size;
		cnt++;
	}

	return iov;
}

/****************************************************/
void IOC::objRdmaPushToClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	if (segments.size() == 2)
		segments.pop_back();
	//build iovec
	iovec * iov = buildIovec(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	size_t size = clientMessage->data.objReadWrite.size;

	//dispaly
	//printf("IOV SIZE %lu (%zu, %zu)\n", segments.size(), clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	//int cnt = 0;
	//for (auto it : segments)
	//	printf("-> %p %zu %zu (%p %zu)\n", it.ptr, it.offset, it.size, iov[cnt].iov_base, iov[cnt++].iov_len);

	//emit rdma write vec & implement callback
	connection.rdmaWritev(clientId, iov, segments.size(), clientMessage->data.objReadWrite.iov.addr, clientMessage->data.objReadWrite.iov.key, [size, &connection, clientId](void){
		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
		msg->header.clientId = 0;
		msg->data.response.msgHasData = false;
		msg->data.response.msgDataSize = 0;
		msg->data.response.status = 0;

		//stats
		gblReadSize += size;

		//send ack message
		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](void){
			delete msg;
			return false;
		});
		
		return false;
	});

	//remove temp
	delete [] iov;
}

/****************************************************/
void IOC::objEagerPushToClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//size
	size_t dataSize = clientMessage->data.objReadWrite.size;

	//send open
	//char * buffer = new char[sizeof(LibfabricMessage) + dataSize];
	LibfabricMessage * msg = (LibfabricMessage *)connection.getDomain().getMsgBuffer();
	//LibfabricMessage * msg = (LibfabricMessage*)buffer;
	msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
	msg->header.clientId = 0;
	msg->data.response.msgDataSize = dataSize;
	msg->data.response.msgHasData = true;
	msg->data.response.status = 0;

	//get base pointer
	char * data = (char*)(msg + 1);

	//copy data
	size_t cur = 0;
	for (auto segment : segments) {
		//compute copy size to stay in data limits
		size_t copySize = segment.size;
		if (cur + copySize > dataSize)
			copySize = dataSize - cur;

		//copy
		memcpy(data + cur, segment.ptr, copySize);

		//progress
		cur += copySize;
	}

	//stats
	gblReadSize += dataSize;

	//send ack message
	connection.sendMessage(msg, sizeof (*msg) + dataSize, clientId, [&connection, msg](void){
		connection.getDomain().retMsgBuffer(msg);
		return false;
	});
}

/****************************************************/
void IOC::setupObjRead(LibfabricConnection & connection, Container & container)
{
	//register hook
	connection.registerHook(IOC_LF_MSG_OBJ_READ, [&connection, &container](int clientId, size_t id, void * buffer) {
		//printf
		//printf("Get OBJ_READ %d\n", clientId);

		//do rdma write on remote segment to send data to reader
		LibfabricMessage * clientMessage = (LibfabricMessage *)buffer;

		//get buffers from object
		Object & object = container.getObject(clientMessage->data.objReadWrite.low, clientMessage->data.objReadWrite.high);
		ObjectSegmentList segments;
		object.getBuffers(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);

		//eager or rdma
		if (clientMessage->data.objReadWrite.size <= IOC_EAGER_MAX_READ) {
			objEagerPushToClient(connection, clientId, clientMessage, segments);
		} else {
			objRdmaPushToClient(connection, clientId, clientMessage, segments);
		}

		//republish
		connection.repostRecive(id);

		return false;
	});
}

/****************************************************/
void IOC::objRdmaFetchFromClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//build iovec
	iovec * iov = buildIovec(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	size_t size = clientMessage->data.objReadWrite.size;

	//emit rdma write vec & implement callback
	connection.rdmaReadv(clientId, iov, segments.size(), clientMessage->data.objReadWrite.iov.addr, clientMessage->data.objReadWrite.iov.key, [size, &connection, clientId, iov](void){
		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
		msg->header.clientId = 0;
		msg->data.response.msgHasData = false;
		msg->data.response.msgDataSize = 0;
		msg->data.response.status = 0;

		//stats
		gblWriteSize += size;

		//send ack message
		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](void){
			delete msg;
			return false;
		});
		
		return false;
	});

	//remove temp
	delete [] iov;
}

/****************************************************/
void IOC::objEagerExtractFromMessage(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//get base pointer
	char * data = (char*)(clientMessage + 1);

	//copy data
	size_t cur = 0;
	size_t dataSize = clientMessage->data.objReadWrite.size;
	for (auto segment : segments) {
		//compute copy size to stay in data limits
		size_t copySize = segment.size;
		if (cur + copySize > dataSize)
			copySize = dataSize - cur;

		//copy
		memcpy(segment.ptr, data + cur, copySize);

		//progress
		cur += copySize;
	}

	//send open
	LibfabricMessage * msg = new LibfabricMessage;
	msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
	msg->header.clientId = 0;
	msg->data.response.status = 0;

	//stats
	gblWriteSize += cur;

	//send ack message
	connection.sendMessage(msg, sizeof (*msg), clientId, [msg](void){
		delete msg;
		return false;
	});
}

/****************************************************/
void IOC::setupObjWrite(LibfabricConnection & connection, Container & container)
{
	//register hook
	connection.registerHook(IOC_LF_MSG_OBJ_WRITE, [&connection, &container](int clientId, size_t id, void * buffer) {
		//printf
		//printf("Get OBJ_READ %d\n", clientId);

		//do rdma write on remote segment to send data to write
		LibfabricMessage * clientMessage = (LibfabricMessage *)buffer;

		//get buffers from object
		Object & object = container.getObject(clientMessage->data.objReadWrite.low, clientMessage->data.objReadWrite.high);
		ObjectSegmentList segments;
		object.getBuffers(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size, false);

		//eager or rdma
		if (clientMessage->data.objReadWrite.msgHasData) {
			objEagerExtractFromMessage(connection, clientId, clientMessage, segments);
		} else {
			objRdmaFetchFromClient(connection, clientId, clientMessage, segments);
		}

		//mark dirty
		object.markDirty(clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);

		//republish
		connection.repostRecive(id);

		return false;
	});
}
