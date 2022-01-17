/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
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
//PLEASE USE HARD CODED VALUE FOR EXPECTED SIZE SO WE CAN DETECT
//IF WE BROKE THE PROTOCOL VERSION BY CHANGING TYPES OR SIZES
template <class T>
void serializeDeserialize(T & in, T & out, size_t expectedSize)
{
	//memset the out struct
	memset(&out, 0, sizeof(out));

	//serialize & dersialize
	char buffer[1024];
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
		.iov = {
			.addr = 0x1,
			.key = 0x2,
		},
		.offset = 10,
		.size = 5,
		.msgHasData = true,
		.optionalData = "01234",
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
		.optionalData = "01234",
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
TEST(TestProtocol, LibfabricFirstHandshake)
{
	//allocate
	LibfabricFirstHandshake out, in = {
		.protocolVersion = 10,
	};

	//apply
	serializeDeserialize(in, out, 4);

	//check
	EXPECT_EQ(in.protocolVersion, out.protocolVersion);
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
