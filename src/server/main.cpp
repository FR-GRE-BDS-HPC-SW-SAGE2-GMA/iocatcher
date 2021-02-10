/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cstdio>
#include <cassert>
#include <cstring>
#include <thread>
#include <argp.h>
//linux
#include <unistd.h>
#include <sys/mman.h>
#include <cstdlib>
#include <sys/uio.h>
//local
#include "core/Container.hpp"
#ifndef NOMERO
	#include "core/clovis_api.h"
#endif
#include "../base/network/LibfabricDomain.hpp"
#include "../base/network/LibfabricConnection.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
static size_t readSize = 0;
static size_t writeSize = 0;

/****************************************************/
const char *argp_program_version = "iocatcher-server";
const char *argp_program_bug_address = "<sebastien.valat@atos.net>";
static char doc[] = "A cache between Mero and ummap-io";
static char args_doc[] = "LISTEN_IP";
static struct argp_option options[] = { 
	{ "nvdimm", 'n', "PATH", 0, "Store data in nvdimm at the given PATH."},
	{ "merofile", 'm', "PATH", 0, "Mero ressource file to use."},
	{ 0 } 
};

/****************************************************/
struct Arguments
{
	Arguments(void);
	const char * listen;
	const char * nvdimm;
	const char * merofile;
};

/****************************************************/
Arguments::Arguments(void)
{
	this->listen = NULL;
	this->nvdimm = NULL;
	this->merofile = "mero_ressource_file.rc";
}

/****************************************************/
error_t parseOptions(int key, char *arg, struct argp_state *state) {
	Arguments *arguments = (Arguments *)state->input;
	switch (key) {
		case 'n': arguments->nvdimm = arg; break;
		case 'l': arguments->listen = arg; break;
		case ARGP_KEY_NO_ARGS: argp_usage (state); break;
		case ARGP_KEY_ARG: 
			if (arguments->listen == NULL)
				arguments->listen = arg;
			else
				argp_usage (state);
			break;
		default: return ARGP_ERR_UNKNOWN;
	}   
	return 0;
}

/****************************************************/
void setupPingPong(LibfabricConnection & connection)
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
		//connection.rdmaWrite(clientId, rmaBuffer, clientMessage->data.iov.addr, clientMessage->data.iov.key, TEST_RDMA_SIZE, new LibfabricPostActionFunction([&connection, clientId](LibfabricPostAction*action){
			//send open
			LibfabricMessage * msg = new LibfabricMessage;
			msg->header.type = IOC_LF_MSG_PONG;
			msg->header.clientId = 0;

			connection.sendMessage(msg, sizeof (*msg), clientId, [msg](LibfabricPostAction*action){
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
void setupObjFlush(LibfabricConnection & connection, Container & container)
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

		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](LibfabricPostAction*action){
			delete msg;
			return false;
		});

		//republish
		connection.repostRecive(id);

		return false;
	});
}

/****************************************************/
void setupObjRegisterRange(LibfabricConnection & connection, Container & container)
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
		if (!tracker.registerRange(clientMessage->data.registerRange.offset, clientMessage->data.registerRange.size, mode))
			status = -1;

		//return message
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_RANGE_REGISTER_ACK;
		msg->header.clientId = clientId;
		msg->data.response.status = status;
		msg->data.response.msgHasData = false;

		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](LibfabricPostAction*action){
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
void setupObjCreate(LibfabricConnection & connection, Container & container)
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

		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](LibfabricPostAction*action){
			delete msg;
			return false;
		});

		//republish
		connection.repostRecive(id);

		return false;
	});
}

/****************************************************/
iovec * buildIovec(ObjectSegmentList & segments, size_t offset, size_t size)
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
void objRdmaPushToClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
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
	connection.rdmaWritev(clientId, iov, segments.size(), clientMessage->data.objReadWrite.iov.addr, clientMessage->data.objReadWrite.iov.key, [size, &connection, clientId](LibfabricPostAction*action){
		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
		msg->header.clientId = 0;
		msg->data.response.msgHasData = false;
		msg->data.response.msgDataSize = 0;
		msg->data.response.status = 0;

		//stats
		readSize += size;

		//send ack message
		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](LibfabricPostAction*action){
			delete msg;
			return false;
		});
		
		return false;
	});

	//remove temp
	delete [] iov;
}

/****************************************************/
void objEagerPushToClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
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
	readSize += dataSize;

	//send ack message
	connection.sendMessage(msg, sizeof (*msg) + dataSize, clientId, [&connection, msg](LibfabricPostAction*action){
		connection.getDomain().retMsgBuffer(msg);
		return false;
	});
}

/****************************************************/
void setupObjRead(LibfabricConnection & connection, Container & container)
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
void objRdmaFetchFromClient(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
{
	//build iovec
	iovec * iov = buildIovec(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
	size_t size = clientMessage->data.objReadWrite.size;

	//emit rdma write vec & implement callback
	connection.rdmaReadv(clientId, iov, segments.size(), clientMessage->data.objReadWrite.iov.addr, clientMessage->data.objReadWrite.iov.key, [size, &connection, clientId, iov](LibfabricPostAction*action){
		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
		msg->header.clientId = 0;
		msg->data.response.msgHasData = false;
		msg->data.response.msgDataSize = 0;
		msg->data.response.status = 0;

		//stats
		writeSize += size;

		//send ack message
		connection.sendMessage(msg, sizeof (*msg), clientId, [msg](LibfabricPostAction*action){
			delete msg;
			return false;
		});
		
		return false;
	});

	//remove temp
	delete [] iov;
}

/****************************************************/
void objEagerExtractFromMessage(LibfabricConnection & connection, int clientId, LibfabricMessage * clientMessage, ObjectSegmentList & segments)
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
	writeSize += cur;

	//send ack message
	connection.sendMessage(msg, sizeof (*msg), clientId, [msg](LibfabricPostAction*action){
		delete msg;
		return false;
	});
}

/****************************************************/
void setupObjWrite(LibfabricConnection & connection, Container & container)
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

/****************************************************/
int main(int argc, char ** argv)
{
	//parse args
	Arguments arguments;
	static struct argp argp = { options, parseOptions, args_doc, doc, 0, 0, 0 };
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	//check
	if (arguments.listen == NULL) {
		fprintf(stderr, "Usage: iocatcher-server {IP}\n");
		return 1;
	} else {
		printf("LISTEN: %s\n", arguments.listen);
	}

	//nvdimm
	if (arguments.nvdimm != NULL) {
		printf("NVDIMM ENABLED: %s\n", arguments.nvdimm);
		Object::setNvdimm(arguments.nvdimm);
	}

	//init mero
	#ifndef NOMERO
		printf("USING MERO RESSOURCE FILE: %s\n", arguments.merofile);
		c0appz_init(0, (char*)arguments.merofile);
	#else
		printf("NOT USING MERO\n");
	#endif

	//stats thread
	bool run = true;
	std::thread statThread = std::thread([&run]{
		while (run) {
			sleep(1);
			printf("Read: %g GB/s, Write: %g GB/s\n", (double)readSize/1.0/1024.0/1024.0/1024.0, (double) writeSize/1.0/1024.0/1024.0/1024.0);
			readSize = 0;
			writeSize = 0;
		}
	});

	//init domain & conn
	LibfabricDomain domain(arguments.listen, "8556", true);
	domain.setMsgBuffeSize(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ));
	LibfabricConnection connection(&domain, false);
	connection.postRecives(1024*1024, 64);

	//test readonly registration
	/*printf("test\n");
	void * ptr = mmap(NULL, 16*1024*1024, PROT_READ, MAP_ANON | MAP_PRIVATE, 0, 0);
	domain.registerSegment(ptr, 16*1024*1024, true, false, false);
	domain.unregisterSegment(ptr, 16*1024*1024);
	munmap(ptr, 16*1024*1024);
	printf("ok\n");*/

	// client lobby
	connection.setHooks([](int id) {
		printf("Get client %d\n", id);
	});
	
	//register read hooks
	Container container(&domain, 8*1024*1024);

	//register hooks
	setupPingPong(connection);
	setupObjRead(connection, container);
	setupObjWrite(connection, container);
	setupObjFlush(connection, container);
	setupObjCreate(connection, container);
	setupObjRegisterRange(connection, container);

	// poll
	for(;;) {
		connection.poll(false);
	}

	//status thread
	run = false;
	statThread.join();

	//close clovis
	#ifndef NOMERO
		c0appz_free();
	#endif
}
