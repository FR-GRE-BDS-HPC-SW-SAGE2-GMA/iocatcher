/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_ACTIONS_H
#define IOC_ACTIONS_H

/****************************************************/
#include "../base/network/LibfabricDomain.hpp"
#include "../base/network/LibfabricConnection.hpp"
#include "../base/network/Protocol.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
void ping_pong(LibfabricDomain & domain, LibfabricConnection &connection, int cnt, size_t eagerSize = 0, size_t rdmaSize = 0);
ssize_t obj_read(LibfabricConnection &connection, const LibfabricObjectId & objectId, void* buffer, size_t size, size_t offset);
ssize_t obj_write(LibfabricConnection &connection, const LibfabricObjectId & objectId, const void* buffer, size_t size, size_t offset);
int obj_flush(LibfabricConnection &connection, const LibfabricObjectId & objectId, size_t offset, size_t size);
int obj_create(LibfabricConnection &connection, const LibfabricObjectId & objectId);
int32_t obj_range_register(LibfabricConnection &connection, const LibfabricObjectId & objectId, size_t offset, size_t size, bool write);
int obj_range_unregister(LibfabricConnection &connection, int32_t id, const LibfabricObjectId & objectId, size_t offset, size_t size, bool write);
int obj_cow(LibfabricConnection &connection, const LibfabricObjectId & sourceObjectId, const LibfabricObjectId & destObjectId, bool allowExist, size_t offset, size_t size);

}

#endif //IOC_ACTIONS_H
