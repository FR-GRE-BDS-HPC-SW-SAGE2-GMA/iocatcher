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
	Server server("127.0.0.1", "8666", true, true);
}

/****************************************************/
TEST(TestServer, run_poll)
{
	Server server("127.0.0.1", "8667", true, true);
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
	Server server("127.0.0.1", "8668", true, true);
	server.startStatsThread();
	usleep(4000);
	server.stop();
}
