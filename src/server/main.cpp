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
//local
#include "core/Container.hpp"
#include "../base/network/LibfabricDomain.hpp"
#include "../base/network/LibfabricConnection.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
void setupPingPong(LibfabricConnection & connection)
{
	//rma
	char * rmaBuffer = new char[TEST_RDMA_SIZE];
	connection.getDomain().registerSegment(rmaBuffer, TEST_RDMA_SIZE);

	//register hook
	connection.registerHook(IOC_LF_MSG_PING, [&connection, rmaBuffer](int clientId, size_t id, void * buffer) {
		//printf
		//printf("Get 10 %d\n", clientId);

		//do rdma read
		LibfabricMessage * clientMessage = (LibfabricMessage *)buffer;
		connection.rdmaWrite(clientId, rmaBuffer, clientMessage->data.iov.addr, clientMessage->data.iov.key, TEST_RDMA_SIZE, new LibfabricPostActionFunction([&connection, clientId](LibfabricPostAction*action){
			//send open
			LibfabricMessage * msg = new LibfabricMessage;
			msg->header.type = IOC_LF_MSG_PONG;
			msg->header.clientId = 0;

			connection.sendMessage(msg, sizeof (*msg), clientId, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
				delete msg;
				return false;
			}));
			
			return false;
		}));

		//republish
		connection.repostRecive(id);

		return false;
	});
}

/****************************************************/
void setupObjRead(LibfabricConnection & connection, Container & container)
{
	//register hook
	connection.registerHook(IOC_LF_MSG_OBJ_READ, [&connection, &container](int clientId, size_t id, void * buffer) {
		//printf
		printf("Get OBJ_READ %d\n", clientId);

		//do rdma write on remote segment to send data to reader
		LibfabricMessage * clientMessage = (LibfabricMessage *)buffer;

		Object & object = container.getObject(clientMessage->data.objReadWrite.low, clientMessage->data.objReadWrite.high);
		ObjectSegmentList segments;
		object.getBuffers(segments, clientMessage->data.objReadWrite.offset, clientMessage->data.objReadWrite.size);
		assert(segments.size() == 1);
		char * rdmaBuffer = segments.front().ptr;
		printf("%p - %lu\n", segments.front().ptr, segments.front().size);

		connection.rdmaWrite(clientId, rdmaBuffer, clientMessage->data.objReadWrite.iov.addr, clientMessage->data.objReadWrite.iov.key, clientMessage->data.objReadWrite.size, new LibfabricPostActionFunction([&connection, clientId](LibfabricPostAction*action){
			//send open
			LibfabricMessage * msg = new LibfabricMessage;
			msg->header.type = IOC_LG_MSG_OBJ_READ_WRITE_ACK;
			msg->header.clientId = 0;
			msg->data.status = 0;

			connection.sendMessage(msg, sizeof (*msg), clientId, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
				delete msg;
				return false;
			}));
			
			return false;
		}));

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

	LibfabricDomain domain(argv[1], "8556", true);
	LibfabricConnection connection(&domain, false);
	connection.postRecives(1024*1024, 64);

	// client lobby
	connection.setHooks([](int id) {
		printf("Get client %d\n", id);
	});
	
	//register read hooks
	Container container(&domain);

	//register hooks
	setupPingPong(connection);
	setupObjRead(connection, container);

	// poll
	for(;;) {
		connection.poll(false);
	}
}
