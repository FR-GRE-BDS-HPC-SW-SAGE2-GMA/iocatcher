/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Protocol.hpp"
#include <sstream>

/****************************************************/
using namespace IOC;

/****************************************************/
// Test the basic constructor.
TEST(TestProtocol, object_id_ostream)
{
	std::stringstream stream;
	LibfabricObjectId objId = {20,10};
	stream << objId;
	ASSERT_EQ("10:20", stream.str());
}
