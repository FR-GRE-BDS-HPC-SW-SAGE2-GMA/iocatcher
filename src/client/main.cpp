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
#include "ioc-client.h"

/****************************************************/
int main(int argc, char ** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: iocatcher-server {IP}\n");
		return 1;
	}

	//setup
	ioc_client_t * client = ioc_client_init(argv[1], "8556");

	//do ping pong
	printf("Run ping pong\n");
	ioc_client_ping_pong(client);

	//create object
	printf("Create obj\n");
	int status = ioc_client_obj_create(client, 10, 20);
	printf("  - Create : %d\n", status);

	//do obj write
	printf("Run obj write\n");
	char * buffer = new char[1024*1024];
	ioc_client_obj_write(client, 10, 20, buffer, 1024*1024, 0);
	ioc_client_obj_write(client, 10, 20, buffer, 1024*1024, 512*1024);

	//do obj read
	printf("Run obj read\n");
	ioc_client_obj_read(client, 10, 20, buffer, 1024*1024, 0);
	ioc_client_obj_read(client, 10, 20, buffer, 1024*1024, 512*1024);
	delete buffer;

	//flush
	printf("Flush object\n");
	ioc_client_obj_flush(client, 10, 20, 0, 0);

	//clear
	ioc_client_fini(client);
}
