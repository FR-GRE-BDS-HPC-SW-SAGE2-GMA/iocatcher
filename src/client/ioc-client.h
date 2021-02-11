/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
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
typedef struct ioc_client_s ioc_client_t;

/****************************************************/
ioc_client_t * ioc_client_init(const char * ip, const char * port);
void ioc_client_fini(ioc_client_t * client);
ssize_t ioc_client_obj_read(ioc_client_t * client, int64_t high, int64_t low, void* buffer, size_t size, size_t offset);
ssize_t ioc_client_obj_write(ioc_client_t * client, int64_t high, int64_t low, const void* buffer, size_t size, size_t offset);
int ioc_client_obj_flush(ioc_client_t * client, int64_t high, int64_t low, uint64_t offset, uint64_t size);
int ioc_client_obj_create(ioc_client_t * client, int64_t high, int64_t low);
void ioc_client_ping_pong(ioc_client_t * client, int cnt);
const char * ioc_client_provider_name(ioc_client_t * client);
void ioc_client_set_passive_wait(ioc_client_t * client, bool value);
int ioc_client_obj_range_register(ioc_client_t * client, int64_t high, int64_t low, size_t offset, size_t size, bool write);
int ioc_client_obj_range_unregister(ioc_client_t * client, int64_t high, int64_t low, size_t offset, size_t size);

/****************************************************/
#ifdef __cplusplus
}
#endif

#endif //IOC_CLIENT_H
