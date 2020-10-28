/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <cstdio>
//local
#include "../base/network/LibfabricDomain.hpp"
#include "../base/network/LibfabricConnection.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
void ping_pong(LibfabricDomain & domain, LibfabricConnection &connection)
{
	//rma
	char * buffer = new char[TEST_RDMA_SIZE];
	memset(buffer, 0, TEST_RDMA_SIZE);
	Iov iov = domain.registerSegment(buffer, TEST_RDMA_SIZE);

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
		connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
			return false;
		}));

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
void obj_read(LibfabricConnection &connection, int64_t low, int64_t high, char* buffer, size_t size, size_t offset)
{
	//register
	Iov iov = connection.getDomain().registerSegment(buffer, size);

	//setup message request
	LibfabricMessage msg;
	msg.header.type = IOC_LF_MSG_OBJ_READ;
	msg.header.clientId = connection.getClientId();
	msg.data.objReadWrite.low = low;
	msg.data.objReadWrite.high = high;
	msg.data.objReadWrite.iov = iov;
	msg.data.objReadWrite.offset = offset;
	msg.data.objReadWrite.size = size;

	//register hook for reception
	LibfabricMessage ackMsg;
	connection.registerHook(IOC_LG_MSG_OBJ_READ_WRITE_ACK, [&connection,&ackMsg](int clientId, size_t id, void * buffer) {
		ackMsg = *(LibfabricMessage *)buffer;
		//printf("get 11 %d\n", clientId);
		connection.repostRecive(id);
		return true;
	});

	//send message
	connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
		return false;
	}));

	//poll
	connection.poll(true);

	//unregister
	connection.getDomain().unregisterSegment(buffer, size);

	//check status
	if (ackMsg.data.status != 0)
		printf("Invalid status : %d\n", ackMsg.data.status);
}

/****************************************************/
void obj_write(LibfabricConnection &connection, int64_t low, int64_t high, char* buffer, size_t size, size_t offset)
{
	//register
	Iov iov = connection.getDomain().registerSegment(buffer, size);

	//setup message request
	LibfabricMessage msg;
	msg.header.type = IOC_LF_MSG_OBJ_WRITE;
	msg.header.clientId = connection.getClientId();
	msg.data.objReadWrite.low = low;
	msg.data.objReadWrite.high = high;
	msg.data.objReadWrite.iov = iov;
	msg.data.objReadWrite.offset = offset;
	msg.data.objReadWrite.size = size;

	//register hook for reception
	LibfabricMessage ackMsg;
	connection.registerHook(IOC_LG_MSG_OBJ_READ_WRITE_ACK, [&connection,&ackMsg](int clientId, size_t id, void * buffer) {
		ackMsg = *(LibfabricMessage *)buffer;
		//printf("get 11 %d\n", clientId);
		connection.repostRecive(id);
		return true;
	});

	//send message
	connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
		return false;
	}));

	//poll
	connection.poll(true);

	//unregister
	connection.getDomain().unregisterSegment(buffer, size);

	//check status
	if (ackMsg.data.status != 0)
		printf("Invalid status : %d\n", ackMsg.data.status);
}

/****************************************************/
int main(int argc, char ** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: iocatcher-server {IP}\n");
		return 1;
	}

	LibfabricDomain domain(argv[1], "8556", false);
	LibfabricConnection connection(&domain, false);
	connection.postRecives(1024*1024, 2);

	//join server
	connection.joinServer();

	//do ping pong
	printf("Run ping pong\n");
	ping_pong(domain, connection);

	//do obj read
	printf("Run obj read\n");
	char * buffer = new char[1024*1024];
	obj_read(connection, 0x10, 0x11, buffer, 1024*1024, 0);
	obj_read(connection, 0x10, 0x11, buffer, 1024*1024, 512*1024);

	//do obj read
	printf("Run obj write\n");
	obj_write(connection, 0x10, 0x11, buffer, 1024*1024, 0);
	obj_write(connection, 0x10, 0x11, buffer, 1024*1024, 512*1024);
	delete buffer;
}
