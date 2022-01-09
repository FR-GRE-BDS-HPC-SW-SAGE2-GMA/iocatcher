/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Serializer.hpp"
#include "../Protocol.hpp"
#include <sstream>

/****************************************************/
using namespace IOC;

/****************************************************/
TEST(TestProtocol, object_id_ostream)
{
	std::stringstream stream;
	LibfabricObjectId objId = {20,10};
	stream << objId;
	ASSERT_EQ("10:20", stream.str());
}

/****************************************************/
TEST(TestProtocol, Iov_operator_equal_equal)
{
	Iov a = {0x10, 0x20};
	Iov b = {0x10, 0x20};
	Iov c = {0x11, 0x21};
	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a == c);
}

/****************************************************/
TEST(TestProtocol, LibfabricObjectId_operator_equal_equal)
{
	LibfabricObjectId a = {10, 20};
	LibfabricObjectId b = {10, 20};
	LibfabricObjectId c = {11, 21};
	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a == c);
}

/****************************************************/
//PLEASE USE HARD CODED VALUE FOR EXPECTED SIZE SO WE CAN DETECT
//IF WE BROKE THE PROTOCOL VERSION BY CHANGING TYPES OR SIZES
template <class T>
void serializeDeserialize(T & in, T & out, size_t expectedSize)
{
	//memset the out struct
	memset(&out, 0, sizeof(out));

	//check
	EXPECT_LE(sizeof(T), IOC_STRUCT_MAX);

	//serialize & dersialize
	char buffer[1024];
	memset(buffer, 1, 1024);

	//serialize
	Serializer serialize(buffer, 1024);
	serialize.apply("in", in);
	EXPECT_EQ(expectedSize, serialize.getCursor());

	//deserialize
	DeSerializer deserialize(buffer, 1024);
	deserialize.apply("out", out);
	EXPECT_EQ(expectedSize, serialize.getCursor());
}

/****************************************************/
TEST(TestProtocol, Iov)
{
	//allocate
	Iov out, in = {
		.addr = 0x1,
		.key = 0x2,
	};

	//apply
	serializeDeserialize(in, out, 16);

	//check
	EXPECT_EQ(in.addr, out.addr);
	EXPECT_EQ(in.key, out.key);
}

/****************************************************/
TEST(TestProtocol, LibfabricPing)
{
	//allocate
	LibfabricPing out, in = {
		.rdmaSize = 10,
		.eagerSize = 5,
		.rdmaIov = {
			.addr = 0x1,
			.key = 0x2
		},
		.eagerData = "0123"
	};

	//apply
	serializeDeserialize(in, out, 37);

	//check
	EXPECT_EQ(in.rdmaSize, out.rdmaSize);
	EXPECT_EQ(in.eagerSize, out.eagerSize);
	EXPECT_EQ(in.rdmaIov.addr, out.rdmaIov.addr);
	EXPECT_EQ(in.rdmaIov.key, out.rdmaIov.key);
	EXPECT_STREQ(in.eagerData, out.eagerData);
}

/****************************************************/
TEST(TestProtocol, LibfabricObjectId)
{
	//allocate
	LibfabricObjectId out, in = {
		.low = 0x1,
		.high = 0x2,
	};

	//apply
	serializeDeserialize(in, out, 16);

	//check
	EXPECT_EQ(in.low, out.low);
	EXPECT_EQ(in.high, out.high);
}

/****************************************************/
TEST(TestProtocol, LibfabricMessageHeader)
{
	//allocate
	LibfabricMessageHeader out, in = {
		.msgType = IOC_LF_MSG_OBJ_COW,
		.lfClientId = 10,
		.tcpClientId = 20,
		.tcpClientKey = 30,
	};

	//apply
	serializeDeserialize(in, out, 32);

	//check
	EXPECT_EQ(in.msgType, out.msgType);
	EXPECT_EQ(in.lfClientId, out.lfClientId);
	EXPECT_EQ(in.tcpClientId, out.tcpClientId);
	EXPECT_EQ(in.tcpClientKey, out.tcpClientKey);
}

/****************************************************/
TEST(TestProtocol, LibfabricObjReadWriteInfos)
{
	//allocate
	LibfabricObjReadWriteInfos out, in = {
		.objectId = {
			.low = 10,
			.high = 20,
		},
		.iov = {
			.addr = 0x1,
			.key = 0x2,
		},
		.offset = 10,
		.size = 5,
		.msgHasData = true,
		.optionalData = "0123",
	};

	//apply
	serializeDeserialize(in, out, 54);

	//check
	EXPECT_EQ(in.iov.addr, out.iov.addr);
	EXPECT_EQ(in.iov.key, out.iov.key);
	EXPECT_EQ(in.offset, out.offset);
	EXPECT_EQ(in.size, out.size);
	EXPECT_EQ(in.msgHasData, out.msgHasData);
	ASSERT_STREQ(in.optionalData, out.optionalData);
}

/****************************************************/
TEST(TestProtocol, LibfabricObjFlushInfos)
{
	//allocate
	LibfabricObjFlushInfos out, in = {
		.objectId = {
			.low = 20,
			.high = 30,
		},
		.offset = 10,
		.size = 32,
	};

	//apply
	serializeDeserialize(in, out, 32);

	//check
	EXPECT_EQ(in.objectId, out.objectId);
	EXPECT_EQ(in.offset, out.offset);
	EXPECT_EQ(in.size, out.size);
}

/****************************************************/
TEST(TestProtocol, LibfabricObjCreateInfos)
{
	//allocate
	LibfabricObjCreateInfos out, in = {
		.objectId = {
			.low = 20,
			.high = 30,
		},
	};

	//apply
	serializeDeserialize(in, out, 16);

	//check
	EXPECT_EQ(in.objectId, out.objectId);
}

/****************************************************/
TEST(TestProtocol, LibfabricRegisterRange)
{
	//allocate
	LibfabricRegisterRange out, in = {
		.objectId = {
			.low = 33,
			.high = 55,
		},
		.offset = 10,
		.size = 20,
		.write = true,
	};

	//apply
	serializeDeserialize(in, out, 33);

	//check
	EXPECT_EQ(in.objectId, out.objectId);
	EXPECT_EQ(in.offset, out.offset);
	EXPECT_EQ(in.size, out.size);
	EXPECT_EQ(in.write, out.write);
}

/****************************************************/
TEST(TestProtocol, LibfabricUnregisterRange)
{
	//allocate
	LibfabricUnregisterRange out, in = {
		.objectId = {
			.low = 33,
			.high = 55,
		},
		.offset = 10,
		.size = 20,
		.id = 15,
		.write = true,
	};

	//apply
	serializeDeserialize(in, out, 37);

	//check
	EXPECT_EQ(in.objectId, out.objectId);
	EXPECT_EQ(in.offset, out.offset);
	EXPECT_EQ(in.size, out.size);
	EXPECT_EQ(in.write, out.write);
	EXPECT_EQ(in.id, out.id);
}

/****************************************************/
TEST(TestProtocol, LibfabricObjectCow)
{
	//allocate
	LibfabricObjectCow out, in = {
		.sourceObjectId = {
			.low = 10,
			.high = 20,
		},
		.destObjectId = {
			.low = 30,
			.high = 40,
		},
		.allowExist = true,
		.rangeOffset = 50,
		.rangeSize = 15,
	};

	//apply
	serializeDeserialize(in, out, 49);

	//check
	EXPECT_EQ(in.sourceObjectId, out.sourceObjectId);
	EXPECT_EQ(in.destObjectId, out.destObjectId);
	EXPECT_EQ(in.allowExist, out.allowExist);
	EXPECT_EQ(in.rangeOffset, out.rangeOffset);
	EXPECT_EQ(in.rangeSize, out.rangeSize);
}

/****************************************************/
TEST(TestProtocol, LibfabricResponse)
{
	//allocate
	LibfabricResponse out, in = {
		.msgDataSize = 5,
		.status = 10,
		.msgHasData = true,
		.optionalData = "0123",
	};

	//apply
	serializeDeserialize(in, out, 18);

	//check
	EXPECT_EQ(in.msgDataSize, out.msgDataSize);
	EXPECT_EQ(in.status, out.status);
	EXPECT_EQ(in.msgHasData, out.msgHasData);
	EXPECT_STREQ(in.optionalData, out.optionalData);
}

/****************************************************/
TEST(TestProtocol, LibfabricResponse_data_fragments)
{
	//fragments
	char f1[] = "Hello";
	char f2[] = "World";
	LibfabricBuffer fragments[2] = {
		{f1, 5},
		{f2, 6}
	};

	//allocate
	LibfabricResponse out, in = {
		.msgDataSize = 5,
		.status = 10,
		.msgHasData = true,
		.optionalData = NULL,
		.optionalDataFragments = fragments,
		.optionalDataFragmentCount = 2
	};

	//apply
	serializeDeserialize(in, out, 24);

	//check
	EXPECT_EQ(in.msgDataSize, out.msgDataSize);
	EXPECT_EQ(in.status, out.status);
	EXPECT_EQ(in.msgHasData, out.msgHasData);
	EXPECT_STREQ("HelloWorld", out.optionalData);
}

/****************************************************/
TEST(TestProtocol, LibfabricFirstHandshake)
{
	//allocate
	LibfabricFirstHandshake out, in = {
		.protocolVersion = 10,
		.assignLfClientId = 20,
	};

	//apply
	serializeDeserialize(in, out, 12);

	//check
	EXPECT_EQ(in.protocolVersion, out.protocolVersion);
	EXPECT_EQ(in.assignLfClientId, out.assignLfClientId);
}

/****************************************************/
TEST(TestProtocol, LibfabricFirstClientMessage)
{
	//allocate
	LibfabricFirstClientMessage out, in;
	strcpy(in.addr, "192.168.1.1");

	//apply
	serializeDeserialize(in, out, 32);

	//check
	EXPECT_STREQ(in.addr, out.addr);
}

/****************************************************/
TEST(TestProtocol, LibfabricEmpty)
{
	//allocate
	LibfabricEmpty out, in;

	//apply
	serializeDeserialize(in, out, 0);
}

/****************************************************/
TEST(TestProtocol, LibfabricErrorMessage)
{
	//allocate
	LibfabricErrorMessage out, in = {
		.errorMessage = "hello",
	};

	//apply
	serializeDeserialize(in, out, 8 + 6);

	//check
	EXPECT_EQ(in.errorMessage, out.errorMessage);
}
