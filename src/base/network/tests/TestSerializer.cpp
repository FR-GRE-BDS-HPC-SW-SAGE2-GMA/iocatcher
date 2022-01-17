/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Serializer.hpp"
#include <sstream>

/****************************************************/
using namespace IOC;

/****************************************************/
// Test the basic constructor.
TEST(TestSerializerBase, constructor)
{
	char buffer[1024];
	SerializerBase serializer(buffer, 1024, PACK);
}

/****************************************************/
// Test the basic constructor.
TEST(TestSerializerBase, apply_int32_t)
{
	//vars
	char buffer[1024];
	int32_t in1 = 10;
	int32_t in2 = 20;
	int32_t out1 = 0;
	int32_t out2 = 0;

	//serialize
	SerializerBase serializer(buffer, 1024, PACK);
	serializer.apply("int1", in1);
	serializer.apply("int2", in2);

	//check
	EXPECT_EQ(2*sizeof(in1), serializer.getCursor());

	//deserialize
	SerializerBase deserializer(buffer, 1024, UNPACK);
	deserializer.apply("out1", out1);
	deserializer.apply("out2", out2);

	//check
	EXPECT_EQ(2*sizeof(out1), deserializer.getCursor());

	//check
	EXPECT_EQ(in1, out1);
	EXPECT_EQ(in2, out2);
}

/****************************************************/
// Test the basic constructor.
TEST(TestSerializerBase, apply_int64_t)
{
	//vars
	char buffer[1024];
	int64_t in1 = 10;
	int64_t in2 = 20;
	int64_t out1 = 0;
	int64_t out2 = 0;

	//serialize
	SerializerBase serializer(buffer, 1024, PACK);
	serializer.apply("int1", in1);
	serializer.apply("int2", in2);

	//check
	EXPECT_EQ(2*sizeof(in1), serializer.getCursor());

	//deserialize
	SerializerBase deserializer(buffer, 1024, UNPACK);
	deserializer.apply("out1", out1);
	deserializer.apply("out2", out2);

	//check
	EXPECT_EQ(2*sizeof(out1), deserializer.getCursor());

	//check
	EXPECT_EQ(in1, out1);
	EXPECT_EQ(in2, out2);
}

/****************************************************/
// Test the basic constructor.
TEST(TestSerializerBase, apply_uint32_t)
{
	//vars
	char buffer[1024];
	uint32_t in1 = 10;
	uint32_t in2 = 20;
	uint32_t out1 = 0;
	uint32_t out2 = 0;

	//serialize
	SerializerBase serializer(buffer, 1024, PACK);
	serializer.apply("int1", in1);
	serializer.apply("int2", in2);

	//check
	EXPECT_EQ(2*sizeof(in1), serializer.getCursor());

	//deserialize
	SerializerBase deserializer(buffer, 1024, UNPACK);
	deserializer.apply("out1", out1);
	deserializer.apply("out2", out2);

	//check
	EXPECT_EQ(2*sizeof(out1), deserializer.getCursor());

	//check
	EXPECT_EQ(in1, out1);
	EXPECT_EQ(in2, out2);
}

/****************************************************/
// Test the basic constructor.
TEST(TestSerializerBase, apply_uint64_t)
{
	//vars
	char buffer[1024];
	uint64_t in1 = 8ul*1024ul*1024ul*1024ul;
	uint64_t in2 = 16ul*1024ul*1024ul*1024ul;
	uint64_t out1 = 0;
	uint64_t out2 = 0;

	//serialize
	SerializerBase serializer(buffer, 1024, PACK);
	serializer.apply("int1", in1);
	serializer.apply("int2", in2);

	//check
	EXPECT_EQ(2*sizeof(in1), serializer.getCursor());

	//deserialize
	SerializerBase deserializer(buffer, 1024, UNPACK);
	deserializer.apply("out1", out1);
	deserializer.apply("out2", out2);

	//check
	EXPECT_EQ(2*sizeof(out1), deserializer.getCursor());

	//check
	EXPECT_EQ(in1, out1);
	EXPECT_EQ(in2, out2);
}

/****************************************************/
// Test the basic constructor.
TEST(TestSerializerBase, apply_string)
{
	//vars
	char buffer[1024];
	std::string in1 = "hello";
	std::string in2 = "world";
	std::string out1;
	std::string out2;

	//serialize
	SerializerBase serializer(buffer, 1024, PACK);
	serializer.apply("int1", in1);
	serializer.apply("int2", in2);

	//check
	EXPECT_EQ(2* sizeof(uint32_t) + in1.size() + in2.size() + 2, serializer.getCursor());

	//deserialize
	SerializerBase deserializer(buffer, 1024, UNPACK);
	deserializer.apply("out1", out1);
	deserializer.apply("out2", out2);

	//check
	EXPECT_EQ(2* sizeof(uint32_t) + in1.size() + in2.size() + 2, deserializer.getCursor());

	//check
	EXPECT_EQ(in1, out1);
	EXPECT_EQ(in2, out2);
}

/****************************************************/
// Test the basic constructor.
TEST(TestSerializerBase, serializeOrPoint)
{
	//vars
	char buffer[1024];
	const char data1[8] = {0,1,2,3,4,5,6,7};
	const char data2[8] = {8,9,10,11,12,13,14,15};
	const char * inData1 = data1;
	const char * inData2 = data2;
	const char * outData1 = NULL;
	const char * outData2 = NULL;

	//serialize
	SerializerBase serializer(buffer, 1024, PACK);
	serializer.serializeOrPoint("data1", inData1, 8);
	serializer.serializeOrPoint("data2", inData2, 8);

	//check
	EXPECT_EQ(16, serializer.getCursor());

	//deserialize
	SerializerBase deserializer(buffer, 1024, UNPACK);
	deserializer.serializeOrPoint("data1", outData1, 8);
	deserializer.serializeOrPoint("data2", outData2, 8);

	//check
	EXPECT_EQ(16, deserializer.getCursor());

	//check
	EXPECT_EQ(buffer+0, outData1);
	EXPECT_EQ(buffer+8, outData2);
}

/****************************************************/
// Test the basic constructor.
TEST(TestSerialerDeserializer, simple)
{
	//vars
	char buffer[1024];
	uint32_t in1 = 10;
	uint32_t out1 = 0;

	//serialize
	Serializer serializer(buffer, 1024);
	serializer.apply("int1", in1);

	//check
	EXPECT_EQ(sizeof(in1), serializer.getCursor());

	//deserialize
	DeSerializer deserializer(buffer, 1024);
	deserializer.apply("out1", out1);

	//check
	EXPECT_EQ(sizeof(out1), deserializer.getCursor());

	//check
	EXPECT_EQ(in1, out1);
}
