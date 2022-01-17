/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include <thread>
#include "client/ioc-client.h"
#include "server/core/Server.hpp"
#include <gmock/gmock.h>

/****************************************************/
using namespace IOC;
using namespace testing;

/****************************************************/
#define ALIGNEMENT (8UL*1024UL*1024UL)

/****************************************************/
class TestHookObjectWrite : public ::testing::Test
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
			this->server->getContainer().setObjectSegmentsAlignement(ALIGNEMENT);
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
TEST_F(TestHookObjectWrite, eager_comm_bug_brdm4216)
{
	//send message to check
	char buffer[IOC_EAGER_MAX_WRITE];
	memset(buffer, 1, sizeof(buffer));
	ioc_client_obj_write(client, 10, 20, buffer, sizeof(buffer), 2*IOC_EAGER_MAX_WRITE);

	//check data has been written at the right location
	char * ptr = (char*)this->server->getContainer().getObject(ObjectId(10,20)).getUniqBuffer(0,ALIGNEMENT, ACCESS_READ, false);
	ASSERT_NE(nullptr, ptr);

	//check content
	for (size_t i = 0 ; i < ALIGNEMENT ; i++) {
		if (i < 2*IOC_EAGER_MAX_WRITE || i >= 3*IOC_EAGER_MAX_WRITE)
			ASSERT_EQ(0, ptr[i]) << "i=" << i;
		else
			ASSERT_EQ(1, ptr[i]) << "i=" << i;
	}
}
