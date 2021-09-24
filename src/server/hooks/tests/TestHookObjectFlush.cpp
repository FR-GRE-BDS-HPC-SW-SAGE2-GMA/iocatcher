/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
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
class TestHookObjectFlush : public ::testing::Test
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
TEST_F(TestHookObjectFlush, simple_flush)
{
	//set buffer
	char buffer[32];
	memset(buffer, 8, sizeof(buffer));

	//replace backend
	StorageBackendGMock storageBackend;
	this->server->setStorageBackend(&storageBackend);

	//call
	EXPECT_CALL(storageBackend, pread(10, 20, _, ALIGNEMENT, 0)).Times(1).WillOnce(Return(ALIGNEMENT));
	ssize_t ret1 = ioc_client_obj_write(client, 10, 20, buffer, sizeof(buffer), 64 );
	ASSERT_EQ(0, ret1);
	EXPECT_CALL(storageBackend, pwrite(10, 20, _, ALIGNEMENT, 0)).Times(1).WillOnce(Return(ALIGNEMENT));
	ssize_t ret2 = ioc_client_obj_flush(client, 10, 20, 0, 0);
	ASSERT_EQ(0, ret2);

	//replace
	this->server->setStorageBackend(NULL);
}
