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
		std::thread thread;
		virtual void SetUp()
		{
			this->server = new Server("127.0.0.1", "8666", true, true);
			this->thread = std::thread([this](){
				this->server->poll();
			});
			this->server->setOnClientConnect([](int){});
			this->client = ioc_client_init("127.0.0.1", "8666");
		}

		virtual void TearDown()
		{
			this->server->stop();
			ioc_client_fini(this->client);
			delete this->server;
			this->thread.join();
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

	//write object
	ASSERT_FALSE(this->server->getContainer().hasObject(10,20));
	ioc_client_obj_write(client, 10, 20, buffer, size, 0);
	ASSERT_TRUE(this->server->getContainer().hasObject(10,20));

	//check meta
	Object & object = this->server->getContainer().getObject(10,20);
	ObjectSegmentList segments;
	object.getBuffers(segments, 0, size, false);
	ASSERT_EQ(1, segments.size());
	ObjectSegment & segment = (*segments.begin());
	ASSERT_GE(segment.size, size);

	//check content
	char * ptr = (char*)segment.ptr;
	for (int i = 0 ; i < size ; i++)
		ASSERT_EQ(1, ptr[i]) << "index " << i;
}

/****************************************************/
TEST_F(TestClientServer, obj_read)
{
	//setup buffer
	const size_t size = 1024;
	char buffer[size];
	memset(buffer, 0, size);

	//setup object
	Object & object = this->server->getContainer().getObject(10,20);
	ObjectSegmentList segments;
	object.getBuffers(segments, 0, size, false);
	ObjectSegment & segment = (*segments.begin());
	char * ptr = (char*)segment.ptr;
	memset(ptr, 1, size);

	//read object
	ioc_client_obj_read(client, 10, 20, buffer, size, 0);

	//check content
	for (int i = 0 ; i < size ; i++)
		ASSERT_EQ(1, buffer[i]) << "index " << i;
}

/****************************************************/
TEST_F(TestClientServer, obj_register)
{
	//read object
	int id = ioc_client_obj_range_register(this->client, 10, 20, 0, 1024, true);
	ASSERT_EQ(1,id);
	ASSERT_EQ(-1,ioc_client_obj_range_register(this->client, 10, 20, 0, 1024, true));

	//check server state
	Object & object = this->server->getContainer().getObject(10,20);
	EXPECT_TRUE(object.getConsistencyTracker().hasCollision(0, 1024, CONSIST_ACCESS_MODE_WRITE));

	//unregister
	ASSERT_EQ(0, ioc_client_obj_range_unregister(this->client, id, 10, 20, 0, 1024, true));
}
