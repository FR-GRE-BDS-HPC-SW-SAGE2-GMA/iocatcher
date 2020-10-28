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

	LibfabricDomain domain(argv[1], "8556", true);
	LibfabricConnection connection(&domain, false);
	connection.postRecives(1024*1024, 64);

	//rma
	char * rmaBuffer = new char[TEST_RDMA_SIZE];
	domain.registerSegment(rmaBuffer, TEST_RDMA_SIZE);

	connection.setHooks([](int id) {
		printf("Get client %d\n", id);
	});

	//register hook
	connection.registerHook(IOC_LF_MSG_PING, [&connection, rmaBuffer](int clientId, size_t id, void * buffer) {
		//printf
		printf("Get 10 %d\n", clientId);

		//republish
		connection.repostRecive(id);

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

		return false;
	});

	for(;;) {
		connection.poll(false);
	}
}
