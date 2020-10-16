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

	for(;;) {
		connection.poll();
	}
}
