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
#include <thread>
#include <unistd.h>
#include <sys/mman.h>
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

			connection.sendMessage(msg, sizeof (*msg), clientId, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
				delete msg;
				return false;
			}));
			
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
		printf("Get flush object %ld:%ld %lu->%lu\n", clientMessage->data.objFlush.high, clientMessage->data.objFlush.low, clientMessage->data.objFlush.offset, clientMessage->data.objFlush.size);

		//flush object
		Object & object = container.getObject(clientMessage->data.objFlush.low, clientMessage->data.objFlush.high);
		int ret = object.flush(clientMessage->data.objFlush.offset, clientMessage->data.objFlush.size);

		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = IOC_LF_MSG_OBJ_FLUSH_ACK;
		msg->header.clientId = clientId;
		msg->data.status = ret;

		connection.sendMessage(msg, sizeof (*msg), clientId, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
			delete msg;
			return false;
		}));

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

		//build iovec
		iovec * iov = buildIovec(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
		size_t size = clientMessage->data.objReadWrite.size;

		//emit rdma write vec & implement callback
		connection.rdmaWritev(clientId, iov, segments.size(), clientMessage->data.objReadWrite.iov.addr, clientMessage->data.objReadWrite.iov.key, new LibfabricPostActionFunction([size, &connection, clientId](LibfabricPostAction*action){
			//send open
			LibfabricMessage * msg = new LibfabricMessage;
			msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
			msg->header.clientId = 0;
			msg->data.status = 0;

			//stats
			readSize += size;

			//send ack message
			connection.sendMessage(msg, sizeof (*msg), clientId, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
				delete msg;
				return false;
			}));
			
			return false;
		}));

		//remove temp
		delete [] iov;

		//republish
		connection.repostRecive(id);

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

		//build iovec
		iovec * iov = buildIovec(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
		size_t size = clientMessage->data.objReadWrite.size;

		//emit rdma write vec & implement callback
		connection.rdmaReadv(clientId, iov, segments.size(), clientMessage->data.objReadWrite.iov.addr, clientMessage->data.objReadWrite.iov.key, new LibfabricPostActionFunction([size, &connection, clientId, iov](LibfabricPostAction*action){
			//send open
			LibfabricMessage * msg = new LibfabricMessage;
			msg->header.type = IOC_LF_MSG_OBJ_READ_WRITE_ACK;
			msg->header.clientId = 0;
			msg->data.status = 0;

			//stats
			writeSize += size;

			//send ack message
			connection.sendMessage(msg, sizeof (*msg), clientId, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
				delete msg;
				return false;
			}));
			
			return false;
		}));

		//remove temp
		delete [] iov;

		//republish
		connection.repostRecive(id);

		return false;
	});
}

/****************************************************/
int main(int argc, char ** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: iocatcher-server {IP}\n");
		return 1;
	}

	//init mero
	#ifndef NOMERO
		c0appz_init(0, "mero_ressource_file.rc");
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
	LibfabricDomain domain(argv[1], "8556", true);
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
	Container container(&domain);

	//register hooks
	setupPingPong(connection);
	setupObjRead(connection, container);
	setupObjWrite(connection, container);
	setupObjFlush(connection, container);

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
