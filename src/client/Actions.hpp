/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_ACTIONS_H
#define IOC_ACTIONS_H

/****************************************************/
#include "../base/network/LibfabricDomain.hpp"
#include "../base/network/LibfabricConnection.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
void ping_pong(LibfabricDomain & domain, LibfabricConnection &connection);
ssize_t obj_read(LibfabricConnection &connection, int64_t high, int64_t low, void* buffer, size_t size, size_t offset);
ssize_t obj_write(LibfabricConnection &connection, int64_t high, int64_t low, const void* buffer, size_t size, size_t offset);
int obj_flush(LibfabricConnection &connection, int64_t high, int64_t low, size_t offset, size_t size);
int obj_create(LibfabricConnection &connection, int64_t high, int64_t low);

}

#endif //IOC_ACTIONS_H
