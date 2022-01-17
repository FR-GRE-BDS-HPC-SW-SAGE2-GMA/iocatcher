/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../ClientRegistry.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
// Test the basic constructor.
TEST(TestClientRegistry, constructor)
{
	ClientRegistry registry;
}

/****************************************************/
// Register a client and check various identities.
TEST(TestClientRegistry, register_check)
{
	//create
	ClientRegistry registry;
	registry.registerClient(10, 123456789);

	//check
	ASSERT_TRUE(registry.checkIdentification(10, 123456789));
	ASSERT_FALSE(registry.checkIdentification(9, 123456789));
	ASSERT_FALSE(registry.checkIdentification(10, 6789));
}

/****************************************************/
// Register two clients, check identities and distconnect then check again.
TEST(TestClientRegistry, disconnect)
{
	//create
	ClientRegistry registry;
	registry.registerClient(10, 123456789);
	registry.registerClient(11, 223456789);

	//check
	ASSERT_TRUE(registry.checkIdentification(10, 123456789));
	ASSERT_TRUE(registry.checkIdentification(11, 223456789));
	
	//disconnect one
	registry.disconnectClient(10);

	//check again
	ASSERT_FALSE(registry.checkIdentification(10, 123456789));
	ASSERT_TRUE(registry.checkIdentification(11, 223456789));
}
