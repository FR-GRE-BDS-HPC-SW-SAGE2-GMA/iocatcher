/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_CLIENT_H
#define IOC_CLIENT_H

/****************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/****************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/****************************************************/
/**
 * Opaque type used to identify a client connection and used
 * to make IOC operations.
**/
typedef struct ioc_client_s ioc_client_t;

/****************************************************/
/**
 * Initialize the IOC client connection so we can make operations
 * to the server.
 * @param ip IP of the IOC server. This should be an IB IP if we 
 * want to perform RDMA operations otherwise it will automatically 
 * fallback to TCP transferts.
 * @param port Server port to connect on.
 * @return Pointer to the ioc_client_t structure of NULL in case of failure.
**/
ioc_client_t * ioc_client_init(const char * ip, const char * port);
/**
 * Terminate a connection to the IOC server.
 * @param client Client connection to close.
**/
void ioc_client_fini(ioc_client_t * client);
/**
 * Make read read operation to read an object from the server.
 * @param client Reference to the client connection handler to use.
 * @param high High part of the object ID to read.
 * @param low low part of the object ID to read.
 * @param buffer Buffer where to place the data. It will be automatically registered for RDMA operation and de-registered after the operation.
 * @param size Size of the read operation.
 * @param offset Offset of the data to read from the object.
 * @return Size of the read operation or negative value on error.
**/
ssize_t ioc_client_obj_read(ioc_client_t * client, int64_t high, int64_t low, void* buffer, size_t size, size_t offset);
/**
 * Make read write operation to write to an object on the server.
 * @param client Reference to the client connection handler to use.
 * @param high High part of the object ID to write.
 * @param low Low part of the object ID to write.
 * @param buffer Buffer where to place the data. It will be automatically registered for RDMA operation and de-registered after the operation.
 * @param size Size of the write operation.
 * @param offset Offset of the data where to write in the object.
 * @return Size of the write operation or negative value on error.
**/
ssize_t ioc_client_obj_write(ioc_client_t * client, int64_t high, int64_t low, const void* buffer, size_t size, size_t offset);
/**
 * Perform a flush operation on a range space of the given object.
 * @param client Reference to the client connection handler to use.
 * @param high High part of the object ID to flush.
 * @param low Low part of the object ID to flush.
 * @param offset Offset of the flush operation.
 * @param size Size of the segment to flush. Can use 0 to say all.
 * @return Return 0 on success, negative value on error.
**/
int ioc_client_obj_flush(ioc_client_t * client, int64_t high, int64_t low, uint64_t offset, uint64_t size);
/**
 * Create an object, this is mostly use to create an object in Mero before starting
 * to flush data to it otherwise it fails.
 * @param client Reference to the client connection handler to use.
 * @param high High part of the object ID.
 * @param low Low part of the object ID.
**/
int ioc_client_obj_create(ioc_client_t * client, int64_t high, int64_t low);
/**
 * Implement the ping pong operation for the client side. We can ask to make the loop
 * as many time as wanted.
 * @param client Reference to the client connection handler to use.
 * @param connection Reference to the client connection to be used.
 * @param cnt Number of time to make the ping pong roundtrip.
**/
void ioc_client_ping_pong(ioc_client_t * client, int cnt);
/**
 * Perform a range registrion to notify we make a mapping on this part of the object.
 * This is used to prevent coherency issue by mapping in write mode several time the
 * same object.
 * @param client Reference to the client connection handler to use.
 * @param high High part of the object ID.
 * @param low Low part of the object ID.
 * @param offset Base offset of the mapping range to register.
 * @param size Size of the mapping range to register.
 * @param write Say if it read or read-write mapping.
 * @return An identifier to be used for de-registration to identify the mapping. It return negative value on error.
**/
int32_t ioc_client_obj_range_register(ioc_client_t * client, int64_t high, int64_t low, size_t offset, size_t size, bool write);
/**
 * Deregister a mapping range to we can reuse it for other mappings.
 * @param client Reference to the client connection handler to use.
 * @param id If of the range to de-register.
 * @param high High part of the object ID.
 * @param low Low part of the object ID.
 * @param offset Base offset of the mapping range to register.
 * @param size Size of the mapping range to register.
 * @param write Say if it read or read-write mapping.
 * @return Return 0 on success and negative value on error.
**/
int ioc_client_obj_range_unregister(ioc_client_t * client, int32_t id, int64_t high, int64_t low, size_t offset, size_t size, bool write);
/**
 * Return the libfabric network provider name to be used.
 * @param client Reference to the client connection handler to use.
 * @return A pointer to a C string.
**/
const char * ioc_client_provider_name(ioc_client_t * client);
/**
 * Enable of disable the passive waiting mode. It disabled it will used active polling.
 * @param client Reference to the client connection handler to configure.
 * @param value Define the new wanted state.
**/
void ioc_client_set_passive_wait(ioc_client_t * client, bool value);
/**
 * Make a copy on write of an object.
 * @param client Reference to the client connection handler to configure.
 * @param orig_high High part of the original object ID.
 * @param orig_low Low part of the origianl object ID.
 * @param dest_high High part of the destination object ID.
 * @param dest_how Low part of the destination object ID.
 * @param alllow_exist Allow the destincation object to already exist.
 * @param offset The offset from which to cow.
 * @param size The size of the range to cow. 0 will reset the object and copy the full original object.
**/
int ioc_client_obj_cow(ioc_client_t * client, int64_t orig_high, int64_t orig_low, int64_t dest_high, int64_t dest_low, bool allow_exist, size_t offset, size_t size);

/****************************************************/
#ifdef __cplusplus
}
#endif

#endif //IOC_CLIENT_H
