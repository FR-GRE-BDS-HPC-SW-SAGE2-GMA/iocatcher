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
void ping_pong(LibfabricDomain & domain, LibfabricConnection &connection, int cnt);
ssize_t obj_read(LibfabricConnection &connection, int64_t high, int64_t low, void* buffer, size_t size, size_t offset);
ssize_t obj_write(LibfabricConnection &connection, int64_t high, int64_t low, const void* buffer, size_t size, size_t offset);
int obj_flush(LibfabricConnection &connection, int64_t high, int64_t low, size_t offset, size_t size);
int obj_create(LibfabricConnection &connection, int64_t high, int64_t low);
int32_t obj_range_register(LibfabricConnection &connection, int64_t high, int64_t low, size_t offset, size_t size, bool write);
int obj_range_unregister(LibfabricConnection &connection, int32_t id, int64_t high, int64_t low, size_t offset, size_t size, bool write);
int obj_cow(LibfabricConnection &connection, int64_t orig_high, int64_t orig_low, int64_t dest_high, int64_t dest_low);

}

#endif //IOC_ACTIONS_H
