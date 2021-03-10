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
/** Maximal size of a libfabric address. **/
#define IOC_LF_MAX_ADDR_LEN 32
//TMP
//#define TEST_RDMA_SIZE (4*1024*1024)
/** 
 * This is an old variable used for ping pong test, need to be 
 * removed as we do not do RDMA anymore for this test.
**/
#define TEST_RDMA_SIZE (4096)
/**
 * Max eager size for write operation. After this do RDMA.
**/
#define IOC_EAGER_MAX_WRITE (0*256*1024)
/**
 * Max eager size for read operation. After this ro RDMA.
**/
#define IOC_EAGER_MAX_READ (0*256*1024)

/****************************************************/
/**
 * Define the type of messages we can exhange via connection.
 * @todo We should split between low level protocol and app implementation
 * protocol.
**/
enum LibfabricMessageType
{
	/** We establish a connection and the client send this as first message. **/
	IOC_LF_MSG_CONNECT_INIT,
	/** As a response to IOC_LF_MSG_CONNECT_INIT the server send ID assignement. **/
	IOC_LF_MSG_ASSIGN_ID,
	/** If recive a message with wrong auth the server will respond with this message. **/
	IOC_LF_MSG_BAD_AUTH,
	/** Limit of the low level protocol IDs, after this value this is app protocol IDs. **/
	IOC_LF_MSG_MAX,
	/** Message ping for ping pong (send by client). **/
	IOC_LF_MSG_PING = 10,
	/** Aswser to ping by the server. **/
	IOC_LF_MSG_PONG,
	/** The client send this message to ask for a read operatoin on the given object. **/
	IOC_LF_MSG_OBJ_READ = 100,
	/** The client send this message to ask for a write operatoin on the given object. **/
	IOC_LF_MSG_OBJ_WRITE,
	/** Aswer by the server to a object read or write operation. **/
	IOC_LF_MSG_OBJ_READ_WRITE_ACK,
	/** Client ask to the server to flush an object given an range. **/
	IOC_LF_MSG_OBJ_FLUSH,
	/** Answer by the server to a flush operation to notify it is one. **/
	IOC_LF_MSG_OBJ_FLUSH_ACK,
	/** Client ask to the server to create an object. **/
	IOC_LF_MSG_OBJ_CREATE,
	/** Server notify the client that the requested object has been created. **/
	IOC_LF_MSG_OBJ_CREATE_ACK,
	/** Client ask to register a rand access to forbit multiple write mapping on the same range. **/
	IOC_LF_MSG_OBJ_RANGE_REGISTER,
	/** Server ack the range registration to the client. **/
	IOC_LF_MSG_OBJ_RANGE_REGISTER_ACK,
	/** Client ask for de-registration of the range. **/
	IOC_LF_MSG_OBJ_RANGE_UNREGISTER,
	/** Server ack for the de-registration of the requested range. **/
	IOC_LF_MSG_OBJ_RANGE_UNREGISTER_ACK,
	/** Make a copy on write of an object **/
	IOC_LF_MSG_OBJ_COW,
	/** The answer for the copy on write operation.**/
	IOC_LF_MSG_OBJ_COW_ACK,
};

/****************************************************/
/**
 * Use to describe the necessary informations to make a RDMA transfer.
 * @brief Informations for an RDMA transfer.
**/
struct Iov
{
	/** Base address of the targetted memory region. **/
	void * addr;
	/** Authentication key to make an RDMA on the given memory region. **/
	uint64_t key;
};

/****************************************************/
/**
 * Message header used by the low level protocol to identify the client and define
 * the mesage type. This is send in head of every message.
**/
struct LibfabricMessageHeader
{
	/** Define the type of message **/
	int type;
	/** Define the client ID. **/
	int clientId;
	/** Define the TCP client ID for auth validation. **/
	uint64_t tcpClientId;
	/** Define the TCP client key for auth validation. **/
	uint64_t tcpClientKey;
};

/****************************************************/
/**
 * Message informations used to make a read/write request on objects.
**/
struct LibfabricObjReadWriteInfos
{
	/** Low part of the object ID. **/
	int64_t low;
	/** High part of the object ID. **/
	int64_t high;
	/** Description of the memory region the read from or write to so the server can make an RDMA operation. **/
	Iov iov;
	/** Offset in the object. **/
	uint64_t offset;
	/** Size of the operation. **/
	uint64_t size;
	/** If the message is an eager message and self contain the data to write (no need of RDMA operation). **/
	bool msgHasData;
};

/****************************************************/
/**
 * Message information for a flush operation.
**/
struct LibfabricObjFlushInfos
{
	/** Low part of the object ID. **/
	int64_t low;
	/** High part of the object ID. **/
	int64_t high;
	/** Offset of the flush operation. **/
	uint64_t offset;
	/** Size of the flush opration (can be 0 to say flush all). **/
	uint64_t size;
};

/****************************************************/
/**
 * Message information to create a new object.
**/
struct LibfabricObjCreateInfos
{
	/** Low part of the object ID. **/
	int64_t low;
	/** High part of the object ID. **/
	int64_t high;
};

/****************************************************/
/**
 * Message information to register a mapping on the given range.
**/
struct LibfabricRegisterRange
{
	/** Low part of the object ID. **/
	int64_t low;
	/** High part of the object ID. **/
	int64_t high;
	/** Offset of the range. **/
	uint64_t offset;
	/** Size of the range. **/
	uint64_t size;
	/** Define if it is a read or read-write mapping. **/
	bool write;
};

/****************************************************/
/**
 * Message to unregister a mapping range. 
**/
struct LibfabricUnregisterRange
{
	/** Low part of the object ID. **/
	int64_t low;
	/** High part of the object ID. **/
	int64_t high;
	/** Offset of the range to unregister. **/
	uint64_t offset;
	/** Size of the range to unregister. **/
	uint64_t size;
	/** ID of the range to unregister. **/
	int32_t id;
	/** If it is a read or read-write range registration. **/
	bool write;
};

/****************************************************/
/**
 * Message used to copy on write an object.
**/
struct LibfabricObjectCow
{
	/** Low part of the origin object ID. **/
	int64_t origLow;
	/** High part of the origin object ID. **/
	int64_t origHigh;
	/** Low part of the destination object ID. **/
	int64_t destLow;
	/** High part of the destination object ID. **/
	int64_t destHigh;
};

/****************************************************/
/**
 * Answer to most of the messages.
 * It can contain information about possible eager data contained following this
 * header.
**/
struct LibfabricResponse
{
	/** Size of the possible extra data placed after the LibfabricMessage header. **/
	uint64_t msgDataSize;
	/** Status of the requested operation. **/
	int32_t status;
	/** Define if the message contain eager data or not. **/
	bool msgHasData;
};

/****************************************************/
/**
 * Global definition of a message. 
 **/
struct LibfabricMessage
{
	/** Header send by all messages. **/
	LibfabricMessageHeader header;
	union {
		/** Client to address to be send to the server on MSG_CONN_INIT **/
		char addr[IOC_LF_MAX_ADDR_LEN];
		/** Used for ping pong tests when using RDMA operation. **/
		Iov iov;
		/** Response to be send after most operations. **/
		LibfabricResponse response;
		/** Infos to make read/write operation on objects. **/
		LibfabricObjReadWriteInfos objReadWrite;
		/** Infos to make a flush operation on an object. **/
		LibfabricObjFlushInfos objFlush;
		/** Infos to create an object. **/
		LibfabricObjCreateInfos objCreate;
		/** Infos to register a mapping range. **/
		LibfabricRegisterRange registerRange;
		/** Infos to register a mapping range. **/
		LibfabricUnregisterRange unregisterRange;
		/** Info to make a copy on write of an object. **/
		LibfabricObjectCow objCow;
	} data;
};

}

#endif //IOC_PROTOCOL_HPP
