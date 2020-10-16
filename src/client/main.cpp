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
	LibfabricDomain domain("127.0.0.1", "8556", false);
	LibfabricConnection connection(&domain, true);
	connection.postRecives(1024*1024, 2);

	//join server
	connection.joinServer();

	//send open
	LibfabricMessage msg;
	msg.header.type = 10;
	msg.header.clientId = connection.getClientId();

	//register hook
	connection.registerHook(11, [&connection](int clientId, size_t id, void * buffer) {
		printf("get 11 %d\n", clientId);
		connection.repostRecive(id);
		return true;
	});

	//send
	connection.sendMessage(&msg, sizeof (msg), IOC_LF_SERVER_ID, new LibfabricPostActionFunction([msg](LibfabricPostAction*action){
		return false;
	}));

	//poll
	connection.poll();
}
