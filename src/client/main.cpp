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
int main(int argc, char ** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: iocatcher-server {IP}\n");
		return 1;
	}

	LibfabricDomain domain(argv[1], "8556", false);
	LibfabricConnection connection(&domain, false);
	connection.postRecives(1024*1024, 2);

	//rma
	char * buffer = new char[TEST_RDMA_SIZE];
	memset(buffer, 0, TEST_RDMA_SIZE);
	Iov iov = domain.registerSegment(buffer, TEST_RDMA_SIZE);

	//join server
	connection.joinServer();

	//send open
	LibfabricMessage msg;
	msg.header.type = IOC_LF_MSG_PING;
	msg.header.clientId = connection.getClientId();
	msg.data.iov = iov;

	//register hook
	connection.registerHook(IOC_LF_MSG_PONG, [&connection](int clientId, size_t id, void * buffer) {
		printf("get 11 %d\n", clientId);
		connection.repostRecive(id);
		return true;
	});

	//time
	struct timespec start, stop;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//send
	int cnt = 10;
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
}
