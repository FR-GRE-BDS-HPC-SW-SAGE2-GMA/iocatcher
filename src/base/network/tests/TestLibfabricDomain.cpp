/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include <thread>
#include <cstring>
#include "../LibfabricDomain.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
// Test basic construction.
TEST(TestLibfaricDomain, constructor)
{
	LibfabricDomain domain("127.0.0.1", "8555", true);
}

/****************************************************/
// Test getting buffers already registered for sendMessage.
TEST(TestLibfaricDomain, getMsgBuffer)
{
	LibfabricDomain domain("127.0.0.1", "8555", true);
	domain.setMsgBufferSize(1024);
	void * ptr = domain.getMsgBuffer();
	domain.retMsgBuffer(ptr);
}

/****************************************************/
// Test getting buffers already registered for sendMessage.
TEST(TestLibfaricDomain, register_mem_and_getmr)
{
	//build domain
	LibfabricDomain domain("127.0.0.1", "8555", true);

	//set buffers
	char buffer1[1024];
	char buffer2[1024];

	//register buffers
	domain.registerSegment(buffer1, 1024, true, true, false);
	domain.registerSegment(buffer2, 1024, true, true, false);

	//get key
	fid_mr * mr1 = domain.getFidMR(buffer1, 1024);
	fid_mr * mr2 = domain.getFidMR(buffer2, 1024);

	//check
	EXPECT_NE(mr1, mr2);
	EXPECT_NE(mr1->mem_desc, mr2->mem_desc);

	//deregister
	domain.unregisterSegment(buffer1, 1024);
	domain.unregisterSegment(buffer2, 1024);
}
