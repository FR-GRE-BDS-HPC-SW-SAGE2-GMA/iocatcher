/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include <unistd.h>
#include "server/core/Server.hpp"
#include "client/ioc-client.h"

/****************************************************/
using namespace IOC;

/****************************************************/
class TestClientServer : public ::testing::Test
{
	protected:
		Server * server;
		ioc_client_t * client;
		Config config;
		std::thread thread;
		virtual void SetUp()
		{
			static int port = 9666;
			char p[16];
			sprintf(p, "%d", port);
			port += 4;
			config.initForUnitTests();
			this->server = new Server(&config, p);
			this->thread = std::thread([this](){
				this->server->poll();
			});
			this->server->setOnClientConnect([](int){});
			this->client = ioc_client_init("127.0.0.1", p);
		}

		virtual void TearDown()
		{
			ioc_client_fini(this->client);
			this->server->stop();
			this->thread.join();
			delete this->server;
		}
};

/****************************************************/
TEST_F(TestClientServer, constructor)
{
}

/****************************************************/
TEST_F(TestClientServer, pingpong)
{
	ioc_client_ping_pong(client, 100);
}

/****************************************************/
TEST_F(TestClientServer, obj_write)
{
	//setup buffer
	const size_t size = 1024;
	char buffer[size];
	memset(buffer, 1, size);

	//object ID
	ObjectId objectId(10, 20);

	//write object
	ASSERT_FALSE(this->server->getContainer().hasObject(objectId));
	ioc_client_obj_write(client, 10, 20, buffer, size, 0);
	ASSERT_TRUE(this->server->getContainer().hasObject(objectId));

	//check meta
	Object & object = this->server->getContainer().getObject(objectId);
	ObjectSegmentList segments;
	object.getBuffers(segments, 0, size, ACCESS_WRITE, false);
	ASSERT_EQ(1, segments.size());
	ObjectSegmentDescr & segment = (*segments.begin());
	ASSERT_GE(segment.size, size);

	//check content
	char * ptr = (char*)segment.ptr;
	for (size_t i = 0 ; i < size ; i++)
		ASSERT_EQ(1, ptr[i]) << "index " << i;
}

/****************************************************/
TEST_F(TestClientServer, obj_write_more_256_obj_segments)
{
	//setup buffer
	const size_t size = 1024*1024;
	const size_t segCnt = 512;
	const size_t segSize = size / segCnt;
	char * buffer = new char[size];
	memset(buffer, 1, size);

	//object ID
	ObjectId objectId(10, 20);

	//set alignement to 0
	this->server->getContainer().setObjectSegmentsAlignement(0);

	//write multiple segments object
	for (size_t i = 0 ; i < size ; i += segSize)
		ioc_client_obj_write(client, 10, 20, buffer+i, segSize, i);

	//check meta
	Object & object = this->server->getContainer().getObject(objectId);
	ObjectSegmentList segments;
	object.getBuffers(segments, 0, size, ACCESS_WRITE, false);
	ASSERT_EQ(segCnt, segments.size());
	ObjectSegmentDescr & segment = (*segments.begin());
	ASSERT_GE(segment.size, segSize);

	//send one
	memset(buffer, 2, size);
	ioc_client_obj_write(client, 10, 20, buffer, size, 0);

	//check content
	for (auto it : segments) {
		char * ptr = (char*)it.ptr;
		for (size_t i = 0 ; i < segSize ; i++)
			ASSERT_EQ(2, ptr[i]) << "index " << i;
	}
}

/****************************************************/
TEST_F(TestClientServer, obj_read)
{
	//setup buffer
	const size_t size = 1024;
	char buffer[size];
	memset(buffer, 0, size);

	//object ID
	ObjectId objectId(10, 20);

	//setup object
	Object & object = this->server->getContainer().getObject(objectId);
	ObjectSegmentList segments;
	object.getBuffers(segments, 0, size, ACCESS_WRITE, false);
	ObjectSegmentDescr & segment = (*segments.begin());
	char * ptr = (char*)segment.ptr;
	memset(ptr, 1, size);

	//read object
	ioc_client_obj_read(client, 10, 20, buffer, size, 0);

	//check content
	for (size_t i = 0 ; i < size ; i++)
		ASSERT_EQ(1, buffer[i]) << "index " << i;
}

/****************************************************/
TEST_F(TestClientServer, obj_read_more_256_obj_segments)
{
	//setup buffer
	const size_t size = 1024*1024;
	const size_t segCnt = 512;
	const size_t segSize = size / segCnt;
	char * buffer = new char[size];
	memset(buffer, 0, size);

	//object ID
	ObjectId objectId(10, 20);

	//set alignement to 0
	this->server->getContainer().setObjectSegmentsAlignement(0);

	//write multiple segments object
	Object & object = this->server->getContainer().getObject(objectId);
	for (size_t i = 0 ; i < size ; i += segSize) {
		ObjectSegmentList segments;
		object.getBuffers(segments, i, segSize, ACCESS_READ, false);
		ASSERT_EQ(1, segments.size());
		memset(segments.front().ptr, 1, segSize);
	}

	//check meta
	ObjectSegmentList segments;
	object.getBuffers(segments, 0, size, ACCESS_READ, false);
	ASSERT_EQ(segCnt, segments.size());
	ObjectSegmentDescr & segment = (*segments.begin());
	ASSERT_EQ(segment.size, segSize);

	//send one
	memset(buffer, 2, size);
	ioc_client_obj_read(client, 10, 20, buffer, size, 0);

	//check content
	for (size_t i = 0 ; i < size ; i++)
		ASSERT_EQ(1, buffer[i]) << "index " << i;
}

/****************************************************/
TEST_F(TestClientServer, obj_register)
{
	//object ID
	ObjectId objectId(10, 20);

	//read object
	int id = ioc_client_obj_range_register(this->client, 10, 20, 0, 1024, true);
	ASSERT_EQ(1,id);
	ASSERT_EQ(-1,ioc_client_obj_range_register(this->client, 10, 20, 0, 1024, true));

	//check server state
	Object & object = this->server->getContainer().getObject(objectId);
	EXPECT_TRUE(object.getConsistencyTracker().hasCollision(0, 1024, CONSIST_ACCESS_MODE_WRITE));

	//unregister
	ASSERT_EQ(0, ioc_client_obj_range_unregister(this->client, id, 10, 20, 0, 1024, true));
}

/****************************************************/
TEST_F(TestClientServer, obj_cow)
{
	//setup buffer
	const size_t size = 1024;
	char buffer[size];
	ssize_t res;
	int status;
	memset(buffer, 1, size);

	//write object
	res = ioc_client_obj_write(client, 10, 20, buffer, size, 0);
	ASSERT_EQ(0, res);

	//make copy on write
	status = ioc_client_obj_cow(client, 10, 20, 10, 21, true);
	ASSERT_EQ(0, status);

	//read again cow object to check content
	char buffer2[size];
	ioc_client_obj_read(client, 10, 21, buffer2, size, 0);
	ASSERT_EQ(0, res);

	//check content
	for (size_t i = 0 ; i < size ; i++)
		ASSERT_EQ(1, buffer2[i]) << "index " << i;
}
