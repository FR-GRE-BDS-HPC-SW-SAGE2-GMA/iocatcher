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
#include "server/backends/StorageBackendGMock.hpp"
#include <gmock/gmock.h>

/****************************************************/
using namespace IOC;
using namespace testing;

/****************************************************/
#define ALIGNEMENT (8UL*1024UL*1024UL)

/****************************************************/
class TestHookObjectCreate : public ::testing::Test
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
TEST_F(TestHookObjectCreate, simple_create)
{
	Container & container = this->server->getContainer();
	ASSERT_FALSE(container.hasObject(ObjectId(10,20)));
	ioc_client_obj_create(client, 10, 20);
	ASSERT_TRUE(container.hasObject(ObjectId(10,20)));
}
