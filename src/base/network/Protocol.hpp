/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

/** This file provide the glue to call daqpipe debug functions **/

#ifndef IOC_PROTOCOL_HPP
#define IOC_PROTOCOL_HPP

/****************************************************/
#include <from-cern-lhcb-daqpipe-v2/Debug.hpp>

/****************************************************/
namespace IOC
{

/****************************************************/
#define IOC_LF_MAX_ADDR_LEN 32
//TMP
//#define TEST_RDMA_SIZE (4*1024*1024)
#define TEST_RDMA_SIZE (4096)

/****************************************************/
enum LibfabricMessageType
{
	IOC_LF_MSG_CONNECT_INIT,
	IOC_LF_MSG_ASSIGN_ID,
	IOC_LF_MSG_MAX,
	IOC_LF_MSG_PING = 10,
	IOC_LF_MSG_PONG,
	IOC_LF_MSG_READ_OBJ = 100,
	IOC_LF_MSG_WRITE_OBJ,
};

/****************************************************/
struct LibfabricMessageHeader
{
	int type;
	int clientId;
};

/****************************************************/
struct LibfabricMessage
{
	LibfabricMessageHeader header;
	union {
		/** Client to address to be send to the server on MSG_CONN_INIT **/
		char addr[IOC_LF_MAX_ADDR_LEN];
		Iov iov;
	} data;
};

}

#endif //IOC_PROTOCOL_HPP
