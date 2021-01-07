/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_CLIENT_H
#define IOC_CLIENT_H

/****************************************************/
#include <vector>
#include <mutex>
#include <cassert>
#include <cstdlib>
#include "Actions.hpp"

/****************************************************/
using namespace IOC;

extern "C"
{

/****************************************************/
struct ioc_client_t
{
	LibfabricDomain * domain;
	std::mutex connections_mutex;
	std::vector<LibfabricConnection *> connections;
};

/****************************************************/
static void ioc_client_ret_connection(ioc_client_t * client, LibfabricConnection * connection)
{
	//check
	assert(client != NULL);
	assert(connection != NULL);

	//setup
	connection->setUsed(false);
}

/****************************************************/
static LibfabricConnection * ioc_client_get_connection(ioc_client_t * client)
{
	//check
	assert(client != NULL);

	//lock
	std::lock_guard<std::mutex> take_lock(client->connections_mutex);

	//search one available
	for (auto it : client->connections) {
		if (it->getUsed() == false) {
			it->setUsed(true);
			return it;
		}
	}

	//setup connection
	LibfabricConnection * connection = new LibfabricConnection(client->domain, false);
	connection->postRecives(2*4096, 2);
	connection->joinServer();
	connection->setUsed(true);

	//add
	client->connections.push_back(connection);

	//ret
	return connection;
}

/****************************************************/
struct ioc_client_t * ioc_client_init(const char * ip, const char * port)
{
	//disable insternal cache which make a leak with ummap-io
	int status = setenv("FI_MR_CACHE_MONITOR", "disabled", 1);
	assume(status == 0, "Fail to set FI_MR_CACHE_MONITOR variable !");

	//init client
	ioc_client_t * client = new ioc_client_t;
	client->domain = new LibfabricDomain(ip, port, false);
	client->domain->setMsgBuffeSize(sizeof(LibfabricMessage));

	//return
	return client;
}

/****************************************************/
void ioc_client_fini(ioc_client_t * client)
{
	client->connections_mutex.lock();
	for (auto it : client->connections)
		delete it;
	delete client->domain;
	delete client;
}

/****************************************************/
ssize_t ioc_client_obj_read(struct ioc_client_t * client, int64_t high, int64_t low, void* buffer, size_t size, size_t offset)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	ssize_t ret = obj_read(*connection, high, low, buffer, size, offset);
	ioc_client_ret_connection(client, connection);
	return ret;
}

/****************************************************/
ssize_t ioc_client_obj_write(struct ioc_client_t * client, int64_t high, int64_t low, const void* buffer, size_t size, size_t offset)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	ssize_t ret = obj_write(*connection, high, low, buffer, size, offset);
	ioc_client_ret_connection(client, connection);
	return ret;
}

/****************************************************/
void ioc_client_ping_pong(ioc_client_t * client)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	ping_pong(*client->domain, *connection);
	ioc_client_ret_connection(client, connection);
}

/****************************************************/
int ioc_client_obj_flush(struct ioc_client_t * client, int64_t high, int64_t low, uint64_t offset, uint64_t size)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	int ret = obj_flush(*connection, high, low, offset, size);
	ioc_client_ret_connection(client, connection);
	return ret;
}

}

#endif //IOC_CLIENT_H
