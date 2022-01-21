/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/** This file provide the glue to call daqpipe debug functions **/

#ifndef IOC_PROTOCOL_HPP
#define IOC_PROTOCOL_HPP

/****************************************************/
//std
#include <ostream>
//internal
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
#define IOC_EAGER_MAX_WRITE (32*1024)
/**
 * Max eager size for read operation. After this ro RDMA.
**/
#define IOC_EAGER_MAX_READ (32*1024)
/**
 * Define the considered max size of the struct.
 */
#define IOC_STRUCT_MAX (64)
/**
 * Post receive to have enough room for eager read
 */
#define IOC_POST_RECEIVE_READ (sizeof(LibfabricMessageHeader) + IOC_STRUCT_MAX + IOC_EAGER_MAX_READ)
/**
 * Post receive to have enough room for eager write
 */
#define IOC_POST_RECEIVE_WRITE (sizeof(LibfabricMessageHeader) + IOC_STRUCT_MAX + IOC_EAGER_MAX_WRITE)
/**
 * Define the protocol version
**/
#define IOC_LF_PROTOCOL_VERSION 2

/****************************************************/
class SerializerBase;

/****************************************************/
/**
 * Define the type of messages we can exhange via connection.
 * @todo We should split between low level protocol and app implementation
 * protocol.
**/
enum LibfabricMessageType
{
	/** We establish a connection and the client sends this as first message. **/
	IOC_LF_MSG_CONNECT_INIT,
	/** As a response to IOC_LF_MSG_CONNECT_INIT the server send ID assignement. **/
	IOC_LF_MSG_ASSIGN_ID,
	/** If receive a message with wrong auth the server will respond with this message. **/
	IOC_LF_MSG_BAD_AUTH,
	/** Message saying an error happen on the server and lead to a crash. **/
	IOC_LF_MSG_FATAL_ERROR,
	/** Limit of the low level protocol IDs, after this value this is app protocol IDs. **/
	IOC_LF_MSG_MAX,
	/** Message ping for ping pong (send by client). **/
	IOC_LF_MSG_PING = 10,
	/** Aswser to ping by the server. **/
	IOC_LF_MSG_PONG,
	/** The client sends this message to ask for a read operation on the given object. **/
	IOC_LF_MSG_OBJ_READ = 100,
	/** The client sends this message to ask for a write operation on the given object. **/
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
typedef uint64_t LibfabricAddr;

/****************************************************/
/**
 * Define a buffer with its size.
**/
struct LibfabricBuffer
{
	void * buffer;
	size_t size;
};

/****************************************************/
/**
 * Define an empty struct to be serialized if we have no data to send
 * in the message.
**/
struct LibfabricEmpty
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	//no content
};

/****************************************************/
/**
 * Use to describe the necessary informations to make a RDMA transfer.
 * @brief Informations for an RDMA transfer.
**/
struct Iov
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** Base address of the targetted memory region. **/
	LibfabricAddr addr;
	/** Authentication key to make an RDMA on the given memory region. **/
	uint64_t key;
};

/****************************************************/
/**
 * Message to make a ping pong operation between the client and the server.
 * The client send a ping with some data and the server responds a pong without
 * data. The data are sent as eager in the message and as RDMA.
 * @brief Structure for the ping pong.
**/
struct LibfabricPing
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** Data to be fetched by the server via RDMA. **/
	uint64_t rdmaSize;
	/** Size of the eager communication (embedded in the request message). **/
	uint64_t eagerSize;
	/** Iov to make the rdma. **/
	Iov rdmaIov;
	/** Pointer to the eager data. **/
	const char * eagerData;
};

/****************************************************/
/**
 * Define an object ID for all operations.
**/
struct LibfabricObjectId
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** Low part of the object ID. **/
	int64_t low;
	/** High part of the object ID. **/
	int64_t high;
};

/****************************************************/
/**
 * Message header used by the low level protocol to identify the client and define
 * the mesage type. This is send in head of every message.
**/
struct LibfabricMessageHeader
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** Define the type of message **/
	uint64_t msgType;
	/** Define the libfabric client ID. **/
	uint64_t lfClientId;
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
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** The object ID to read or write **/
	LibfabricObjectId objectId;
	/** Description of the memory region the read from or write to so the server can make an RDMA operation. **/
	Iov iov;
	/** Offset in the object. **/
	uint64_t offset;
	/** Size of the operation. **/
	uint64_t size;
	/** If the message is an eager message and self contain the data to write (no need of RDMA operation). **/
	bool msgHasData;
	/** Optional data. **/
	const char * optionalData;
};

/****************************************************/
/**
 * Message information for a flush operation.
**/
struct LibfabricObjFlushInfos
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** The obejct ID to flus. **/
	LibfabricObjectId objectId;
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
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** The object ID to create. **/
	LibfabricObjectId objectId;
};

/****************************************************/
/**
 * Message information to register a mapping on the given range.
**/
struct LibfabricRegisterRange
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** The object ID for which to register range. **/
	LibfabricObjectId objectId;
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
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** The object ID for which to de-register range. **/
	LibfabricObjectId objectId;
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
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** The ID of the source. **/
	LibfabricObjectId sourceObjectId;
	/** The ID of the destication. */
	LibfabricObjectId destObjectId;
	/** If allow object to already exist. **/
	bool allowExist;
	/** Offset from which to cow **/
	uint64_t rangeOffset;
	/** Size of the range to cow **/
	uint64_t rangeSize;
};

/****************************************************/
/**
 * Answer to most of the messages.
 * It can contain information about possible eager data contained following this
 * header.
**/
struct LibfabricResponse
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** Basic setup. **/
	inline void initStatusOnly(int32_t status);
	/** Size of the possible extra data placed after the LibfabricMessage header. **/
	uint64_t msgDataSize;
	/** Status of the requested operation. **/
	int32_t status;
	/** Define if the message contain eager data or not. **/
	bool msgHasData;
	/** pointer to the optional data. **/
	const char * optionalData;
	/**
	 * Optional data in a multi-fragment representation (will go in optionalData on deserialize).
	 * NULL if none, an array of optionalDataFragmentCount elements otherwise.
	 * If used optionalData should be NULL.
	**/
	const LibfabricBuffer * optionalDataFragments;
	/** Number of fragments. **/
	uint32_t optionalDataFragmentCount;
};

/****************************************************/
/**
 * Structure used on libfabric connection establishiment to return
 * server informations like protocol verison.
**/
struct LibfabricFirstHandshake
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** Define the protocol version in use by the server. **/
	int32_t protocolVersion;
	/** Define the client ID to assign. **/
	uint64_t assignLfClientId;
};

/****************************************************/
struct LibfabricFirstClientMessage
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** Address of the client. **/
	char addr[IOC_LF_MAX_ADDR_LEN];
};

/****************************************************/
struct LibfabricErrorMessage
{
	/** Used to serialize and de-serialize the struct **/
	inline void applySerializerDef(SerializerBase & serializer);
	/** Contain the error message. **/
	std::string errorMessage;
};

/****************************************************/
std::ostream & operator <<(std::ostream & out, const LibfabricObjectId & objectId);
std::ostream & operator <<(std::ostream & out, const Iov & iov);
bool operator==(const LibfabricObjectId & a, const LibfabricObjectId & b);
bool operator==(const Iov & a, const Iov & b);
}

/****************************************************/
#include "Protocol_inlined.hpp"

#endif //IOC_PROTOCOL_HPP
