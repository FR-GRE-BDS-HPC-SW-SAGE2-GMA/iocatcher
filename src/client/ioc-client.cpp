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
#include <semaphore.h>
#include "Actions.hpp"
#include "../base/network/TcpClient.hpp"
#include "ioc-client.h"

/****************************************************/
using namespace IOC;

/****************************************************/
#define IOC_CLIENT_MAX_CONN 8

extern "C"
{

/****************************************************/
struct ioc_client_s
{
	ioc_client_s(void);
	LibfabricDomain * domain;
	TcpClient * tcpClient;
	TcpConnInfo clientConnInfo;
	std::mutex connections_mutex;
	sem_t connections_semaphore;
	std::vector<LibfabricConnection *> connections;
	bool passive_wait;
};
typedef ioc_client_s ioc_client_t;

/****************************************************/
ioc_client_s::ioc_client_s(void)
{
	this->domain = NULL;
	this->tcpClient = NULL;
	this->passive_wait = true;
}

/****************************************************/
static void ioc_client_ret_connection(ioc_client_t * client, LibfabricConnection * connection)
{
	//check
	assert(client != NULL);
	assert(connection != NULL);

	//setup
	connection->setUsed(false);

	//open one again
	sem_post(&client->connections_semaphore);
}

/****************************************************/
static LibfabricConnection * ioc_client_get_connection(ioc_client_t * client)
{
	//check
	assert(client != NULL);

	//limit number
	sem_wait(&client->connections_semaphore);

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
	LibfabricConnection * connection = new LibfabricConnection(client->domain, client->passive_wait);
	connection->setTcpClientInfos(client->clientConnInfo.clientId, client->clientConnInfo.key);
	connection->postRecives(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_READ), 2);
	connection->joinServer();
	connection->setUsed(true);

	//add
	client->connections.push_back(connection);

	//ret
	return connection;
}

/****************************************************/
ioc_client_t * ioc_client_init(const char * ip, const char * port)
{
	//disable insternal cache which make a leak with ummap-io
	int status = setenv("FI_MR_CACHE_MONITOR", "disabled", 1);
	assume(status == 0, "Fail to set FI_MR_CACHE_MONITOR variable !");

	//init client
	ioc_client_t * client = new ioc_client_t;

	//establish tcp connection
	char tcpPort[16];
	sprintf(tcpPort, "%d", atoi(port) + 1);
	client->tcpClient = new TcpClient(ip, tcpPort);
	client->clientConnInfo = client->tcpClient->getConnectionInfos();

	//setup domain
	client->domain = new LibfabricDomain(ip, port, false);
	//client->domain->setMsgBuffeSize(sizeof(LibfabricMessage));
	client->domain->setMsgBuffeSize(sizeof(LibfabricMessage)+(IOC_EAGER_MAX_WRITE));
	client->passive_wait = true;

	//init semaphore
	sem_init(&client->connections_semaphore, 0, IOC_CLIENT_MAX_CONN);

	//return
	return client;
}

/****************************************************/
void ioc_client_fini(ioc_client_t * client)
{
	//wait all to came back
	for (int i = 0 ; i < IOC_CLIENT_MAX_CONN ; i++)
		sem_wait(&client->connections_semaphore);

	//start clean
	client->connections_mutex.lock();
	for (auto it : client->connections)
		delete it;
	delete client->domain;
	delete client->tcpClient;
	delete client;
	sem_destroy(&client->connections_semaphore);
}

/****************************************************/
ssize_t ioc_client_obj_read(ioc_client_t * client, int64_t high, int64_t low, void* buffer, size_t size, size_t offset)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	ssize_t ret = obj_read(*connection, high, low, buffer, size, offset);
	ioc_client_ret_connection(client, connection);
	return ret;
}

/****************************************************/
ssize_t ioc_client_obj_write(ioc_client_t * client, int64_t high, int64_t low, const void* buffer, size_t size, size_t offset)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	ssize_t ret = obj_write(*connection, high, low, buffer, size, offset);
	ioc_client_ret_connection(client, connection);
	return ret;
}

/****************************************************/
void ioc_client_ping_pong(ioc_client_t * client, int cnt)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	ping_pong(*client->domain, *connection, cnt);
	ioc_client_ret_connection(client, connection);
}

/****************************************************/
int ioc_client_obj_flush(ioc_client_t * client, int64_t high, int64_t low, uint64_t offset, uint64_t size)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	int ret = obj_flush(*connection, high, low, offset, size);
	ioc_client_ret_connection(client, connection);
	return ret;
}

/****************************************************/
int ioc_client_obj_create(ioc_client_t * client, int64_t high, int64_t low)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	int ret = obj_create(*connection, high, low);
	ioc_client_ret_connection(client, connection);
	return ret;
}

/****************************************************/
const char * ioc_client_provider_name(ioc_client_t * client)
{
	return client->domain->getLFProviderName();
}

/****************************************************/
void ioc_client_set_passive_wait(ioc_client_t * client, bool value)
{
	client->passive_wait = value;
}

/****************************************************/
int32_t ioc_client_obj_range_register(ioc_client_t * client, int64_t high, int64_t low, size_t offset, size_t size, bool write)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	int ret = obj_range_register(*connection, high, low, offset, size, write);
	ioc_client_ret_connection(client, connection);
	return ret;
}

/****************************************************/
int ioc_client_obj_range_unregister(ioc_client_t * client, int32_t id, int64_t high, int64_t low, size_t offset, size_t size, bool write)
{
	LibfabricConnection * connection = ioc_client_get_connection(client);
	int ret = obj_range_unregister(*connection, id, high, low, offset, size, write);
	ioc_client_ret_connection(client, connection);
	return ret;
}

}

#endif //IOC_CLIENT_H
