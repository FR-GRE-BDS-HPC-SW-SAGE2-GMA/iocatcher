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
#define IOC_EAGER_MAX_WRITE (0*256*1024)
#define IOC_EAGER_MAX_READ (0*256*1024)

/****************************************************/
enum LibfabricMessageType
{
	IOC_LF_MSG_CONNECT_INIT,
	IOC_LF_MSG_ASSIGN_ID,
	IOC_LF_MSG_MAX,
	IOC_LF_MSG_PING = 10,
	IOC_LF_MSG_PONG,
	IOC_LF_MSG_OBJ_READ = 100,
	IOC_LF_MSG_OBJ_WRITE,
	IOC_LF_MSG_OBJ_READ_WRITE_ACK,
	IOC_LF_MSG_OBJ_FLUSH,
	IOC_LF_MSG_OBJ_FLUSH_ACK,
	IOC_LF_MSG_OBJ_CREATE,
	IOC_LF_MSG_OBJ_CREATE_ACK,
};

/****************************************************/
struct LibfabricMessageHeader
{
	int type;
	int clientId;
};

/****************************************************/
struct LibfabricObjReadWriteInfos
{
	int64_t low;
	int64_t high;
	Iov iov;
	uint64_t offset;
	uint64_t size;
	bool msgHasData;
};

/****************************************************/
struct LibfabricObjFlushInfos
{
	int64_t low;
	int64_t high;
	uint64_t offset;
	uint64_t size;
};

/****************************************************/
struct LibfabricObjCreateInfos
{
	int64_t low;
	int64_t high;
};

/****************************************************/
struct LibfabricResponse
{
	int64_t msgDataSize;
	int status;
	bool msgHasData;
};

/****************************************************/
struct LibfabricMessage
{
	LibfabricMessageHeader header;
	union {
		/** Client to address to be send to the server on MSG_CONN_INIT **/
		char addr[IOC_LF_MAX_ADDR_LEN];
		Iov iov;
		LibfabricResponse response;
		LibfabricObjReadWriteInfos objReadWrite;
		LibfabricObjFlushInfos objFlush;
		LibfabricObjCreateInfos objCreate;
	} data;
};

}

#endif //IOC_PROTOCOL_HPP
