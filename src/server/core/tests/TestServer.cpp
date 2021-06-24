/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include <unistd.h>
#include "../Server.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
TEST(TestServer, constructor)
{
	//config
	Config config;
	config.initForUnitTests();

	//server
	Server server(&config, "8666");

	//wait a bit so the tcp thread is in loop()
	usleep(100000);
}

/****************************************************/
TEST(TestServer, run_poll)
{
	//config
	Config config;
	config.initForUnitTests();

	//server
	Server server(&config, "8668");
	std::thread runner([&server]{
		server.poll();
	});
	usleep(4000);
	server.stop();
	runner.join();
}

/****************************************************/
TEST(TestServer, run_stats)
{
	//config
	Config config;
	config.initForUnitTests();

	//server
	Server server(&config, "8670");
	server.startStatsThread();
	usleep(4000);
	server.stop();
}
