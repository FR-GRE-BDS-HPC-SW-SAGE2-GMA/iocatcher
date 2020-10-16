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
	LibfabricDomain domain("127.0.0.1", "8556", true);
	LibfabricConnection connection(&domain, true);
	connection.postRecives(1024*1024, 64);

	connection.setHooks([](int id) {
		printf("Get client %d\n", id);
	});

	//register hook
	connection.registerHook(10, [&connection](int clientId, size_t id, void * buffer) {
		//printf
		printf("Get 10 %d\n", clientId);

		//send open
		LibfabricMessage * msg = new LibfabricMessage;
		msg->header.type = 11;
		msg->header.clientId = 0;

		connection.sendMessage(msg, sizeof (*msg), clientId, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
			delete msg;
			return false;
		}));

		return false;
	});

	for(;;) {
		connection.poll();
	}
}