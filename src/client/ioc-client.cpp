/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_CLIENT_H
#define IOC_CLIENT_H

/****************************************************/
#include "Actions.hpp"

/****************************************************/
using namespace IOC;

extern "C"
{

/****************************************************/
struct ioc_client_t
{
	LibfabricDomain * domain;
	LibfabricConnection * connection;
};

/****************************************************/
struct ioc_client_t * ioc_client_init(const char * ip, const char * port)
{
	ioc_client_t * client = new ioc_client_t;
	client->domain = new LibfabricDomain(ip, port, false);
	client->connection = new LibfabricConnection(client->domain, false);

	//post recive buffers
	client->connection->postRecives(1024*1024, 2);

	//join server
	client->connection->joinServer();

	//return
	return client;
}

/****************************************************/
void ioc_client_fini(ioc_client_t * client)
{
	delete client->connection;
	delete client->domain;
	delete client;
}

/****************************************************/
ssize_t ioc_client_obj_read(struct ioc_client_t * client, int64_t high, int64_t low, void* buffer, size_t size, size_t offset)
{
	return obj_read(*client->connection, high, low, buffer, size, offset);
}

/****************************************************/
ssize_t ioc_client_obj_write(struct ioc_client_t * client, int64_t high, int64_t low, const void* buffer, size_t size, size_t offset)
{
	return obj_write(*client->connection, high, low, buffer, size, offset);
}

/****************************************************/
void ioc_client_ping_pong(ioc_client_t * client)
{
	ping_pong(*client->domain, *client->connection);
}

/****************************************************/
int ioc_client_obj_flush(struct ioc_client_t * client, int64_t high, int64_t low)
{
	return obj_flush(*client->connection, high, low);
}

}

#endif //IOC_CLIENT_H
