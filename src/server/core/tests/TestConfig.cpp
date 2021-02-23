/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Config.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
TEST(TestConfig, options)
{
	Config config;
	const char * argv[] = {"ioc-server", "--nvdimm=/path1,/path2", "127.0.0.1", "\0"};
	config.parseArgs(3, argv);
	EXPECT_EQ(2, config.nvdimmMountPath.size());
	EXPECT_EQ("/path1", config.nvdimmMountPath[0]);
	EXPECT_EQ("/path2", config.nvdimmMountPath[1]);
}
