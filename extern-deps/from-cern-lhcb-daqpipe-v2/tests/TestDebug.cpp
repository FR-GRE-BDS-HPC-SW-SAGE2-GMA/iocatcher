/*****************************************************
             PROJECT  : lhcb-daqpipe
             VERSION  : 2.5.0-dev
             DATE     : 12/2017
             AUTHOR   : Valat Sébastien - CERN
             LICENSE  : CeCILL-C
*****************************************************/

/********************  HEADERS  *********************/
#include <cerrno>
#include <gtest/gtest.h>
#include "../Debug.hpp"

/***************** USING NAMESPACE ******************/
using namespace DAQ;

/*******************  FUNCTION  *********************/
TEST(DEBUG,debug)
{
	DAQ_DEBUG("test","test debug");
	DAQ_DEBUG_ARG("test","test debug %1").arg("value").end();
}

/*******************  FUNCTION  *********************/
TEST(DEBUG,debugCat)
{
	Debug::showCat("test");
	DAQ_DEBUG("test","test debug");
	DAQ_DEBUG_ARG("test","test debug %1").arg("value").end();
	Debug::showList();
	EXPECT_NE(nullptr,(void*)Debug::getCatMap());
}

/*******************  FUNCTION  *********************/
TEST(Debug,showParentsCat_one_level)
{
	EXPECT_FALSE(Debug::showParentsCat("parent1:test1"));
	EXPECT_FALSE(Debug::showParentsCat("parent1.test1"));
	Debug::enableCat("parent1");
	EXPECT_TRUE(Debug::showParentsCat("parent1:test1"));
	EXPECT_TRUE(Debug::showParentsCat("parent1.test1"));
}

/*******************  FUNCTION  *********************/
TEST(Debug,showParentsCat_two_level)
{
	EXPECT_FALSE(Debug::showParentsCat("parent2:parent3:test1"));
	Debug::enableCat("parent2:parent3");
	EXPECT_TRUE(Debug::showParentsCat("parent2:parent3:test1"));
}

/*******************  FUNCTION  *********************/
TEST(DEBUG,warn)
{
	DAQ_WARNING("test debug");
	DAQ_WARNING_ARG("test debug %1").arg("value").end();
}

/*******************  FUNCTION  *********************/
TEST(DEBUG,error)
{
	DAQ_ERROR("test debug");
	DAQ_ERROR_ARG("test debug %1").arg("value").end();
}

/*******************  FUNCTION  *********************/
TEST(DEBUG,fatal)
{
	::testing::FLAGS_gtest_death_test_style = "threadsafe";
	ASSERT_DEATH(DAQ_FATAL("test debug"),"");
	ASSERT_DEATH(DAQ_FATAL_ARG("test debug %1").arg("value").end(),"");
}
