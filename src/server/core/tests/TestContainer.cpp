/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <gtest/gtest.h>
#include "../Container.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
TEST(TestContainer, constructor)
{
	Container container(NULL, NULL);
}

/****************************************************/
TEST(TestContainer, getObject)
{
	Container container(NULL, NULL);
	Object & obj1 = container.getObject(10,20);
	Object & obj2 = container.getObject(10,21);
	Object & obj3 = container.getObject(10,20);

	EXPECT_NE(&obj1, &obj2);
	EXPECT_EQ(&obj1, &obj3);
}

/****************************************************/
TEST(TestContainer, hasObject)
{
	Container container(NULL, NULL);
	ASSERT_FALSE(container.hasObject(10, 20));
	container.getObject(10, 20);
	ASSERT_TRUE(container.hasObject(10, 20));

}

/****************************************************/
TEST(TestContainer, onClientDisconnect)
{
	Container container(NULL, NULL);
	Object & obj1 = container.getObject(10,20);
	Object & obj2 = container.getObject(10,21);

	//client on obj1
	obj1.getConsistencyTracker().registerRange(0, 100, 100, CONSIST_ACCESS_MODE_WRITE);
	obj1.getConsistencyTracker().registerRange(1, 200, 100, CONSIST_ACCESS_MODE_WRITE);
	
	//client on obj2
	obj2.getConsistencyTracker().registerRange(0, 100, 100, CONSIST_ACCESS_MODE_WRITE);
	obj2.getConsistencyTracker().registerRange(1, 200, 100, CONSIST_ACCESS_MODE_WRITE);

	//check
	ASSERT_TRUE(obj1.getConsistencyTracker().hasCollision(100, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(obj1.getConsistencyTracker().hasCollision(200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(obj2.getConsistencyTracker().hasCollision(100, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(obj2.getConsistencyTracker().hasCollision(200, 100, CONSIST_ACCESS_MODE_WRITE));

	//disconnect
	container.onClientDisconnect(0);

	//check
	ASSERT_FALSE(obj1.getConsistencyTracker().hasCollision(100, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(obj1.getConsistencyTracker().hasCollision(200, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_FALSE(obj2.getConsistencyTracker().hasCollision(100, 100, CONSIST_ACCESS_MODE_WRITE));
	ASSERT_TRUE(obj2.getConsistencyTracker().hasCollision(200, 100, CONSIST_ACCESS_MODE_READ));
}
