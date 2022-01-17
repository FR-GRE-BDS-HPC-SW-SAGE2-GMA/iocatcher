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
class TestHookObjectRead : public ::testing::Test
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
TEST_F(TestHookObjectRead, eager_comm_bug_brdm4216)
{
	//fill
	char * ptr = (char*)this->server->getContainer().getObject(ObjectId(10,20)).getUniqBuffer(0,ALIGNEMENT, ACCESS_READ, false);
	ASSERT_NE(nullptr, ptr);
	for (size_t i = 0 ; i < ALIGNEMENT ; i++) {
		if (i < 2*IOC_EAGER_MAX_READ || i >= 3*IOC_EAGER_MAX_READ)
			ptr[i] = 0;
		else
			ptr[i] = 1;
	}

	//send message to check
	char buffer[IOC_EAGER_MAX_READ];
	ioc_client_obj_read(client, 10, 20, buffer, sizeof(buffer), 2*IOC_EAGER_MAX_READ);

	//check content
	for (size_t i = 0 ; i < IOC_EAGER_MAX_READ ; i++)
		ASSERT_EQ(1, buffer[i]) << "i=" << i;
}

/****************************************************/
TEST_F(TestHookObjectRead, invalid)
{
	//replace backend to generate error
	StorageBackendGMock storageBackend;
	this->server->setStorageBackend(&storageBackend);
	EXPECT_CALL(storageBackend, pread(11, 20, _, ALIGNEMENT, 0)).Times(1).WillOnce(Return(-1));

	//send message to check
	char buffer[64];
	ssize_t res = ioc_client_obj_read(client, 11, 20, buffer, sizeof(buffer), 0);
	ASSERT_EQ(-1, res);

	//remove before destroying it
	this->server->setStorageBackend(NULL);
}
