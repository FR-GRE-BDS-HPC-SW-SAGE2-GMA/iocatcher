/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include <thread>
#include <cstring>
#include "../LibfabricDomain.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
TEST(TestLibfaricDomain, constructor)
{
	LibfabricDomain domain("127.0.0.1", "8555", true);
}

/****************************************************/
TEST(TestLibfaricDomain, getMsgBuffer)
{
	LibfabricDomain domain("127.0.0.1", "8555", true);
	domain.setMsgBuffeSize(1024);
	void * ptr = domain.getMsgBuffer();
	domain.retMsgBuffer(ptr);
}
